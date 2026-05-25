#ifndef UTILS_H
#define UTILS_H
#endif

#include <Windows.h>
#include <string>
#include <memory>
#include <functional>

#include <MinHook.h>

#include <RakNet/PacketEnumerations.h>
#include <RakNet/StringCompressor.h>
#include <RakNet/BitStream.h>

template <typename T>
std::string read_with_size(RakNet::BitStream *bs)
{
    T size;
    if (!bs->Read(size))
        return {};
    std::string str(size, '\0');
    bs->Read(str.data(), size);
    return str;
}

template <typename T>
void write_with_size(RakNet::BitStream *bs, std::string_view str)
{
    T size = static_cast<T>(str.size());
    bs->Write(size);
    bs->Write(str.data(), size);
}

template <typename FunctionPtrT>
class c_hook
{
	public:
		c_hook(std::uintptr_t target = 0) : _target(target) { MH_Initialize(); };

		~c_hook() { this->remove(); }

		const char *status() { return MH_StatusToString(_create_status); }

		bool add(FunctionPtrT detour, bool enable = true)
		{
			_create_status = MH_CreateHook(reinterpret_cast<void *>(_target), detour, reinterpret_cast<void **>(&_original));
			if (!enable)
				return _create_status == MH_OK;
			auto enabled = MH_EnableHook(reinterpret_cast<void *>(_target));

			return _create_status == MH_OK && enabled == MH_OK;
		}

		void set_adr(std::uintptr_t target) { _target = target; }

		bool enable() { return MH_EnableHook(reinterpret_cast<void *>(_target)) == MH_OK; }

		FunctionPtrT get_original() { return _original; }

		bool disable() { return MH_DisableHook(reinterpret_cast<void *>(_target)) == MH_OK; }

		bool remove() { return MH_RemoveHook(reinterpret_cast<void *>(_target)) == MH_OK; }

		template <typename... Args>
		auto call_original(Args &&...args) const
		{
			return (*_original)(std::forward<Args>(args)...);
		}

	private:
		MH_STATUS _create_status{MH_UNKNOWN};
		std::uintptr_t _target{0};
		void *_detour{nullptr};
		FunctionPtrT _original{nullptr};
};

class VTableHookManager
{
	public:
		VTableHookManager(void **vTable, unsigned short numFuncs) : m_vTable(vTable), m_numberFuncs(numFuncs)
		{
			m_originalFuncs = new void *[m_numberFuncs]; // allocating memory so we can save here the addresses of the original functions
			for (int i = 0; i < m_numberFuncs; i++) {
				m_originalFuncs[i] = GetFunctionAddyByIndex(i); // saving the address of the original functions
			}
		}

		~VTableHookManager() {
			delete[] m_originalFuncs; // we need to free the allocated memory
		}

		void *GetFunctionAddyByIndex(unsigned short index) // getting the address of a virtual function in the vtable by index
		{	
			if (index < m_numberFuncs) {
				return m_vTable[index];
			} else {
				return nullptr;
			}
		}

		void *Hook(unsigned short index, void *ourFunction) // hooking the virtual function by index
		{
			uintptr_t bufferOriginalFunc = NULL;
			if (!toHook(index, true, ourFunction, &bufferOriginalFunc)) {
				return nullptr;
			}
			return reinterpret_cast<void *>(bufferOriginalFunc);
		}

		bool Unhook(unsigned short index) // unhooking the virtual function by index
		{	
			if (!toHook(index)) { // if not succeded
				return false; // return false
			}
			return true; // else return true
		}

		void UnhookAll()
		{
			for (int index = 0; index < m_numberFuncs; index++) {
				if (m_vTable[index] == m_originalFuncs[index]) {
					continue; // if not hooked skip this index
				}
				Unhook(index);
			}
		}

	private:
		void **m_vTable;				  // the vtable of some object
		unsigned short m_numberFuncs;	  // number of virtual functions
		void **m_originalFuncs = nullptr; // we'll save the original address here

		bool toHook(unsigned short index, bool hook = false, void *ourFunction = nullptr, uintptr_t *bufferOriginalFunc = nullptr)
		{
			DWORD OldProtection = NULL;
			if (index < m_numberFuncs) {
				if (hook) {
					if (!ourFunction || !bufferOriginalFunc) {
						return false;
					}
					*bufferOriginalFunc = (uintptr_t)m_vTable[index]; // saving the original address in our buffer so we can call the function
					VirtualProtect(m_vTable + index, 0x4, PAGE_EXECUTE_READWRITE, &OldProtection);
					m_vTable[index] = ourFunction;
					VirtualProtect(m_vTable + index, 0x4, OldProtection, &OldProtection);
					return true;
				} else {
					VirtualProtect(m_vTable + index, 0x4, PAGE_EXECUTE_READWRITE, &OldProtection);
					m_vTable[index] = m_originalFuncs[index];
					VirtualProtect(m_vTable + index, 0x4, OldProtection, &OldProtection);
					return true;
				}
			}
			return false;
		}
};