#include "memv/memv.h"
#include <protocol/tcp_socket.h>
#include <protocol/frame.h>
#include <protocol/control.h>
#include "memory_generated.h"
#include <cstdio>
#include <thread>
#include <chrono>
#include <cstdlib>

using namespace imp::protocol;

int main(int argc, char** argv)
{
	const u16 port = argc > 1 ? static_cast<u16>( std::atoi(argv[1]) ) : 47810;
	TCPSocket socket;
	if (!socket.connect("127.0.0.1", port))
	{
		printf("memv: failed to connect on port: %d\n", port);
		return 1;
	}
	socket.setNonBlocking(true);

	if (!socket.send(encodeFrame(MessageType::Control, encodeControl(ControlOp::Subscribe, MessageMask::MemoryTelemetry))))
	{
		printf("memv: failed to send subscribe request\n");
		return 1;
	}

	printf("memv: connected, waiting for telemetry...\n");
	FrameReader reader;
	std::vector<u8> chunk;

	while (true)
	{
		chunk.clear();
		if (!socket.recv(chunk))
		{
			printf("memv: engine disconnected");
			break;
		}
		if (!chunk.empty())
			reader.append(chunk);

		while (auto frame = reader.tryExtract())
		{
			if (frame->type != MessageType::MemoryTelemetry) continue;

			const auto* telemetry = memory::GetMemoryTelemetry(frame->payload.data());
			if (!telemetry || !telemetry->allocators()) continue;

			printf("\n<== memory snapshot ==>\n");
			for (const auto* alloc : *telemetry->allocators())
			{
				printf("%s \n	used=%d\n	peak=%d\n", 
					alloc->allocator_name()->c_str(), 
					alloc->current_used(), 
					alloc->peak_used()
				);

				if (const auto* tagBytes = alloc->tag_bytes())
				{
					for (flatbuffers::uoffset_t i = 0; i < tagBytes->size(); ++i)
					{
						const u64 bytes = tagBytes->Get(i);
						if (bytes == 0) continue;

						const auto tag = static_cast<memory::MemTag>(i);
						printf("	%s: %d bytes\n", memory::EnumNameMemTag(tag), bytes);
					}
				}
			}
		}

		if (reader.isPoisoned())
		{
			printf("memv: malformed stream, dropping\n");
			break;
		}

		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}
