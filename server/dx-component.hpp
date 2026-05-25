#pragma once

#include <sdk.hpp>
#include <network.hpp>
#include <Server/Components/Pawn/pawn.hpp>

using namespace Impl;

class DXComponent final
	: public IComponent
	, public PlayerConnectEventHandler
	, public PawnEventHandler
	, public SingleNetworkInEventHandler
{
private:
	ICore* core_ = nullptr;
	IPawnComponent* pawn_ = nullptr;
	inline static DXComponent* instance_ = nullptr;

public:
	PROVIDE_UID(0x4b7c62d0ea8f319bULL);

	// Required component methods.
	StringView componentName() const override;
	SemanticVersion componentVersion() const override;
	void onLoad(ICore* c) override;
	void onInit(IComponentList* components) override;
	void onReady() override;
	void onFree(IComponent* component) override;
	void free() override;
	void reset() override;
	
	// Connect event methods.
	void onPlayerConnect(IPlayer& player) override;
	void onPlayerDisconnect(IPlayer& player, PeerDisconnectReason reason) override;

	// Network event methods (handles RPC 191)
	bool onReceive(IPlayer& peer, NetworkBitStream& bs) override;
	
	// Pawn event methods.
	void onAmxLoad(IPawnScript& script) override;
	void onAmxUnload(IPawnScript& script) override;
	
	static DXComponent* getInstance();
	IPawnComponent* getPawnComponent() const { return pawn_; }

	~DXComponent();
};
