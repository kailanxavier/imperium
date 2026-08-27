#pragma once

#include <core/fs/vfs.h>
#include <ecs/world.h>

#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>

class ScriptFsFixture : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_tempDir = std::filesystem::temp_directory_path() /
			("imp_script_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
				"_" + std::to_string(reinterpret_cast<uintptr_t>(this)));

		std::filesystem::create_directories(m_tempDir);
		ASSERT_TRUE(vfs.mount("scripts", m_tempDir.string(), /*priority=*/0, /*writable=*/true, /*createIfMissing=*/true));
	}

	void TearDown() override
	{
		std::error_code ec;
		std::filesystem::remove_all(m_tempDir, ec);
	}

	std::string writeScript(const std::string& name, const std::string& source)
	{
		std::ofstream out(m_tempDir / name, std::ios::trunc);
		out << source;
		out.close();
		return "scripts/" + name;
	}

	imp::fs::VirtualFileSystem vfs;

private:
	std::filesystem::path m_tempDir;
};
