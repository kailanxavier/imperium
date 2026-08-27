#include <core/fs/vfs.h>
#include <script/file_watcher.h>

#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

using namespace imp::script;
namespace stdfs = std::filesystem;

namespace
{
	void touch(const stdfs::path& path, stdfs::file_time_type time)
	{
		std::error_code ec;
		stdfs::last_write_time(path, time, ec);
		ASSERT_FALSE(ec);
	}

	void writeFile(const stdfs::path& path, const std::string& contents)
	{
		std::ofstream out(path, std::ios::trunc);
		out << contents;
	}
}

class ScriptFileWatcherTest : public ::testing::Test
{
protected:
	void SetUp() override
	{
		m_tempDir = stdfs::temp_directory_path() /
			("imp_watcher_test_" + std::to_string(::testing::UnitTest::GetInstance()->random_seed()) +
				"_" + std::to_string(reinterpret_cast<uintptr_t>(this)));
		stdfs::create_directories(m_tempDir);

		ASSERT_TRUE(vfs.mount("scripts", m_tempDir.string(), 0, true, true));

		m_clock = stdfs::file_time_type::clock::now();
	}

	void TearDown() override
	{
		std::error_code ec;
		stdfs::remove_all(m_tempDir, ec);
	}

	stdfs::file_time_type nextTimestamp()
	{
		m_clock += std::chrono::hours(1);
		return m_clock;
	}

	stdfs::path physicalPath(const std::string& name) const { return m_tempDir / name; }

	imp::fs::VirtualFileSystem vfs;

private:
	stdfs::path m_tempDir;
	stdfs::file_time_type m_clock;
};

TEST_F(ScriptFileWatcherTest, IsInvalidWhenNoMountMatchesThePrefix)
{
	ScriptFileWatcher watcher(vfs, "does_not_exist/");
	EXPECT_FALSE(watcher.isValid());
	EXPECT_TRUE(watcher.poll().empty());
}

TEST_F(ScriptFileWatcherTest, IsValidForAnExistingMount)
{
	ScriptFileWatcher watcher(vfs, "scripts");
	EXPECT_TRUE(watcher.isValid());
}

TEST_F(ScriptFileWatcherTest, FirstPollEstablishesBaselineWithoutReportingExistingFiles)
{
	writeFile(physicalPath("pickup.lua"), "return {}");
	touch(physicalPath("pickup.lua"), nextTimestamp());

	ScriptFileWatcher watcher(vfs, "scripts/");
	EXPECT_TRUE(watcher.poll().empty());
}

TEST_F(ScriptFileWatcherTest, ReportsAFileModifiedAfterTheBaseline)
{
	writeFile(physicalPath("pickup.lua"), "return {}");
	touch(physicalPath("pickup.lua"), nextTimestamp());

	ScriptFileWatcher watcher(vfs, "scripts/");
	ASSERT_TRUE(watcher.poll().empty());

	writeFile(physicalPath("pickup.lua"), "return { OnInit = function() end }");
	touch(physicalPath("pickup.lua"), nextTimestamp());

	const std::vector<std::string> changed = watcher.poll();
	ASSERT_EQ(changed.size(), 1u);
	EXPECT_EQ(changed[0], "scripts/pickup.lua");
}

TEST_F(ScriptFileWatcherTest, DoesNotReportAnUnchangedFileOnSubsequentPolls)
{
	writeFile(physicalPath("pickup.lua"), "return {}");
	touch(physicalPath("pickup.lua"), nextTimestamp());

	ScriptFileWatcher watcher(vfs, "scripts/");
	ASSERT_TRUE(watcher.poll().empty());
	EXPECT_TRUE(watcher.poll().empty());
}

TEST_F(ScriptFileWatcherTest, ReportsANewlyCreatedFileAfterTheBaseline)
{
	ScriptFileWatcher watcher(vfs, "scripts/");
	ASSERT_TRUE(watcher.poll().empty());

	writeFile(physicalPath("platform.lua"), "return {}");
	touch(physicalPath("platform.lua"), nextTimestamp());

	const std::vector<std::string> changed = watcher.poll();
	ASSERT_EQ(changed.size(), 1u);
	EXPECT_EQ(changed[0], "scripts/platform.lua");
}

TEST_F(ScriptFileWatcherTest, IgnoresFilesWithADifferentExtension)
{
	writeFile(physicalPath("notes.txt"), "todo: ship the game");
	touch(physicalPath("notes.txt"), nextTimestamp());

	ScriptFileWatcher watcher(vfs, "scripts/");
	ASSERT_TRUE(watcher.poll().empty());

	writeFile(physicalPath("notes.txt"), "todo: ship the game (updated)");
	touch(physicalPath("notes.txt"), nextTimestamp());

	EXPECT_TRUE(watcher.poll().empty());
}

TEST_F(ScriptFileWatcherTest, RecreatingADeletedFileIsReportedAsAChange)
{
	writeFile(physicalPath("pickup.lua"), "return {}");
	touch(physicalPath("pickup.lua"), nextTimestamp());

	ScriptFileWatcher watcher(vfs, "scripts/");
	ASSERT_TRUE(watcher.poll().empty());

	stdfs::remove(physicalPath("pickup.lua"));
	EXPECT_TRUE(watcher.poll().empty());

	writeFile(physicalPath("pickup.lua"), "return { OnInit = function() end }");
	touch(physicalPath("pickup.lua"), nextTimestamp());

	const std::vector<std::string> changed = watcher.poll();
	ASSERT_EQ(changed.size(), 1u);
	EXPECT_EQ(changed[0], "scripts/pickup.lua");
}

TEST_F(ScriptFileWatcherTest, WatchesFilesInNestedDirectories)
{
	stdfs::create_directories(physicalPath("enemies"));
	writeFile(physicalPath("enemies/goblin.lua"), "return {}");
	touch(physicalPath("enemies/goblin.lua"), nextTimestamp());

	ScriptFileWatcher watcher(vfs, "scripts/");
	ASSERT_TRUE(watcher.poll().empty());

	writeFile(physicalPath("enemies/goblin.lua"), "return { OnInit = function() end }");
	touch(physicalPath("enemies/goblin.lua"), nextTimestamp());

	const std::vector<std::string> changed = watcher.poll();
	ASSERT_EQ(changed.size(), 1u);
	EXPECT_EQ(changed[0], "scripts/enemies/goblin.lua");
}
