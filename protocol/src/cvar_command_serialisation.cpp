#include <protocol/cvar_command.h>
#include "cvar_command_generated.h"
#include <flatbuffers/flatbuffers.h>

namespace imp::protocol
{
    namespace
    {
        flatbuffers::Offset<cvar::CVarEntry> buildEntry(
            flatbuffers::FlatBufferBuilder& builder, const CVarEntryPayload& entry)
        {
            const auto nameOffset = builder.CreateString(entry.name);

            cvar::CVarEntryBuilder eb(builder);
            eb.add_name(nameOffset);
            eb.add_type(static_cast<cvar::CVarType>( entry.type ));
            eb.add_float_value(entry.floatValue);
            eb.add_int_value(entry.intValue);
            eb.add_bool_value(entry.boolValue);
            return eb.Finish();
        }

        CVarEntryPayload readEntry(const cvar::CVarEntry& entry)
        {
            CVarEntryPayload p;
            if (entry.name()) p.name = entry.name()->str();
            p.type = static_cast<CVarType>(entry.type());
            p.floatValue = entry.float_value();
            p.intValue = entry.int_value();
            p.boolValue = entry.bool_value();
            return p;
        }
    }

    std::vector<u8> serialiseCVarCommand(const CVarCommandPayload& cmd)
    {
        flatbuffers::FlatBufferBuilder builder;

        const auto nameOffset = builder.CreateString(cmd.name);

        cvar::CVarCommandBuilder cb(builder);
        cb.add_op(static_cast<cvar::CVarCommandOp>( cmd.op ));
        cb.add_name(nameOffset);
        cb.add_type(static_cast<cvar::CVarType>( cmd.type ));
        cb.add_float_value(cmd.floatValue);
        cb.add_int_value(cmd.intValue);
        cb.add_bool_value(cmd.boolValue);
        builder.Finish(cb.Finish());

        return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
    }

    std::optional<CVarCommandPayload> deserialiseCVarCommand(std::span<const u8> payload)
    {
        flatbuffers::Verifier verifier(payload.data(), payload.size());
        if (!cvar::VerifyCVarCommandBuffer(verifier))
            return std::nullopt;

        const auto* cmd = cvar::GetCVarCommand(payload.data());
        if (!cmd)
            return std::nullopt;

        CVarCommandPayload p;
        p.op = static_cast<CVarCommandOp>( cmd->op() );
        if (cmd->name()) p.name = cmd->name()->str();
        p.type = static_cast<CVarType>( cmd->type() );
        p.floatValue = cmd->float_value();
        p.intValue = cmd->int_value();
        p.boolValue = cmd->bool_value();

        return p;
    }

    std::vector<u8> serialiseCVarCommandResult(const CVarCommandResultPayload& result)
    {
        flatbuffers::FlatBufferBuilder builder;

        const auto nameOffset = builder.CreateString(result.name);
        const auto errorOffset = builder.CreateString(result.error);

        std::vector<flatbuffers::Offset<cvar::CVarEntry>> entryOffsets;
        entryOffsets.reserve(result.entries.size());
        for (const auto& e : result.entries)
            entryOffsets.push_back(buildEntry(builder, e));
        const auto entriesOffset = builder.CreateVector(entryOffsets);

        flatbuffers::Offset<cvar::CVarEntry> entryOffset;
        if (result.entry)
            entryOffset = buildEntry(builder, *result.entry);

        cvar::CVarCommandResultBuilder rb(builder);
        rb.add_op(static_cast<cvar::CVarCommandOp>( result.op ));
        rb.add_name(nameOffset);
        rb.add_success(result.success);
        rb.add_error(errorOffset);
        rb.add_entries(entriesOffset);
        if (result.entry)
            rb.add_entry(entryOffset);
        builder.Finish(rb.Finish());

        return { builder.GetBufferPointer(), builder.GetBufferPointer() + builder.GetSize() };
    }

    std::optional<CVarCommandResultPayload> deserialiseCVarCommandResult(std::span<const u8> payload)
    {
        flatbuffers::Verifier verifier(payload.data(), payload.size());
        if (!verifier.VerifyBuffer<cvar::CVarCommandResult>(nullptr))
            return std::nullopt;

        const auto* result = flatbuffers::GetRoot<cvar::CVarCommandResult>(payload.data());
        if (!result)
            return std::nullopt;

        CVarCommandResultPayload p;
        p.op = static_cast<CVarCommandOp>( result->op() );
        if (result->name()) p.name = result->name()->str();
        p.success = result->success();
        if (result->error()) p.error = result->error()->str();

        if (const auto* entries = result->entries())
        {
            p.entries.reserve(entries->size());
            for (const auto* e : *entries)
            {
                if (!e) continue;
                p.entries.push_back(readEntry(*e));
            }
        }

        if (const auto* entry = result->entry())
            p.entry = readEntry(*entry);

        return p;
    }
}
