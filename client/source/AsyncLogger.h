#pragma once

#include <Windows.h>

#include <condition_variable>
#include <cstddef>
#include <deque>
#include <fstream>
#include <mutex>
#include <string>
#include <thread>

class AsyncLogger {
public:
	explicit AsyncLogger(const char* path)
		: path_(path), worker_(&AsyncLogger::run, this)
	{
	}

	~AsyncLogger()
	{
		{
			std::lock_guard<std::mutex> lock(mutex_);
			stopping_ = true;
		}
		condition_.notify_one();
		if (worker_.joinable()) {
			worker_.join();
		}
	}

	void log(std::string text)
	{
		if (text.size() > MaxMessageBytes) {
			text.resize(MaxMessageBytes);
			text += " [truncated]";
		}
		std::lock_guard<std::mutex> lock(mutex_);
		if (stopping_) {
			return;
		}
		if (queue_.size() >= MaxQueuedMessages) {
			queue_.pop_front();
			++dropped_;
		}
		queue_.push_back(std::move(text));
		condition_.notify_one();
	}

private:
	static constexpr std::size_t MaxQueuedMessages = 4096;
	static constexpr std::size_t MaxMessageBytes = 4096;

	void run()
	{
		std::ofstream file(path_, std::ios::app);
		for (;;) {
			std::deque<std::string> pending;
			std::size_t dropped = 0;
			{
				std::unique_lock<std::mutex> lock(mutex_);
				condition_.wait(lock, [this] { return stopping_ || !queue_.empty(); });
				pending.swap(queue_);
				dropped = dropped_;
				dropped_ = 0;
				if (stopping_ && pending.empty()) {
					break;
				}
			}

			if (!file.is_open()) {
				file.open(path_, std::ios::app);
			}
			if (!file.is_open()) {
				continue;
			}

			if (dropped != 0) {
				writeLine(file, "logger queue full; dropped " + std::to_string(dropped) + " messages");
			}
			for (const std::string& message : pending) {
				writeLine(file, message);
			}
			file.flush();
		}
	}

	static void writeLine(std::ofstream& file, const std::string& text)
	{
		SYSTEMTIME time {};
		GetLocalTime(&time);
		char prefix[64];
		sprintf_s(prefix, "[%02d:%02d:%02d.%03d] ",
			time.wHour, time.wMinute, time.wSecond, time.wMilliseconds);
		file << prefix << text << '\n';
	}

	std::string path_;
	std::mutex mutex_;
	std::condition_variable condition_;
	std::deque<std::string> queue_;
	std::thread worker_;
	std::size_t dropped_ = 0;
	bool stopping_ = false;
};
