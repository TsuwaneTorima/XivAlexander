#pragma once

#include <regex>

#include "Sqex_Sound_Reader.h"
#include "Utils_Win32_Process.h"

namespace Sqex::Sound {

	struct MusicImportTargetChannel {
		std::string source;
		uint32_t channel;
	};

	void from_json(const nlohmann::json& j, MusicImportTargetChannel& o);

	struct MusicImportSegmentItem {
		std::vector<MusicImportTargetChannel> channels;
		std::map<std::string, double> sourceOffsets;
		std::map<std::string, double> sourceThresholds;
		std::map<std::string, std::string> sourceFilters;
		std::optional<double> length;  // in seconds
	};

	void from_json(const nlohmann::json& j, MusicImportSegmentItem& o);

	struct MusicImportTarget {
		std::vector<std::filesystem::path> path;
		std::vector<uint32_t> sequentialToFfmpegChannelIndexMap;
		float loopOffsetDelta;
		int loopLengthDivisor;
		std::vector<MusicImportSegmentItem> segments;
		bool enable;
	};

	void from_json(const nlohmann::json& j, MusicImportTarget& o);

	struct MusicImportSourceItemInputFile {
		std::optional<std::string> directory;
		std::string pattern;
		std::wregex pattern_compiled;
	};

	void from_json(const nlohmann::json& j, MusicImportSourceItemInputFile& o);

	struct MusicImportSourceItem {
		std::vector<std::vector<MusicImportSourceItemInputFile>> inputFiles;
		std::string filterComplex;
		std::string filterComplexOutName;
	};

	void from_json(const nlohmann::json& j, MusicImportSourceItem& o);

	struct MusicImportItem {
		std::map<std::string, MusicImportSourceItem> source;
		std::vector<MusicImportTarget> target;
	};

	void from_json(const nlohmann::json& j, MusicImportItem& o);

	struct MusicImportSearchDirectory {
		bool default_;
		std::map<std::string, std::string> purchaseLinks;
	};

	void from_json(const nlohmann::json& j, MusicImportSearchDirectory& o);

	struct MusicImportConfig {
		std::string name;
		std::map<std::string, MusicImportSearchDirectory> searchDirectories;
		std::vector<MusicImportItem> items;
	};

	void from_json(const nlohmann::json& j, MusicImportConfig& o);

	class MusicImporter {
		class FloatPcmSource {
			Utils::Win32::Process m_hReaderProcess;
			Utils::Win32::Handle m_hStdoutReader;
			Utils::Win32::Thread m_hStdinWriterThread;

			std::vector<float> m_buffer;

		public:
			FloatPcmSource(
				const MusicImportSourceItem& sourceItem,
				std::vector<std::filesystem::path> resolvedPaths,
				std::function<std::span<uint8_t>(size_t len, bool throwOnIncompleteRead)> linearReader, const char* linearReaderType,
				const std::filesystem::path& ffmpegPath, int forceSamplingRate = 0, std::string audioFilters = {}
			);

			~FloatPcmSource();

			std::span<float> operator()(size_t len, bool throwOnIncompleteRead) {
				m_buffer.resize(len);
				return std::span(m_buffer).subspan(0, m_hStdoutReader.Read(0, std::span(m_buffer), throwOnIncompleteRead ? Utils::Win32::Handle::PartialIoMode::AlwaysFull : Utils::Win32::Handle::PartialIoMode::AllowPartial));
			}
		};

		static nlohmann::json RunProbe(const std::filesystem::path& path, const std::filesystem::path& ffprobePath);

		static nlohmann::json RunProbe(const char* originalFormat, std::function<std::span<uint8_t>(size_t len, bool throwOnIncompleteRead)> linearReader, const std::filesystem::path& ffprobePath);

		std::map<std::string, MusicImportSourceItem> m_sourceItems;
		MusicImportTarget m_target;
		const std::filesystem::path m_ffmpeg, m_ffprobe;

		std::map<std::string, std::vector<std::filesystem::path>> m_sourcePaths;
		std::vector<std::shared_ptr<Sqex::Sound::ScdReader>> m_targetOriginals;

		struct SourceSet {
			uint32_t Rate{};
			uint32_t Channels{};

			std::unique_ptr<FloatPcmSource> Reader;
			std::vector<float> ReadBuf;
			size_t ReadBufPtr{};

			std::vector<int> FirstBlocks;
		};
		std::map<std::string, SourceSet> m_sourceInfo;

		static constexpr auto OriginalSource = "target";

	public:
		MusicImporter(std::map<std::string, MusicImportSourceItem> sourceItems, MusicImportTarget target, std::filesystem::path ffmpeg, std::filesystem::path ffprobe);

		void AppendReader(std::shared_ptr<Sqex::Sound::ScdReader> reader);

		bool ResolveSources(std::string dirName, const std::filesystem::path& dir);

		void Merge(const std::function<void(const std::filesystem::path& path, std::vector<uint8_t>)>& cb);
	};

}
