#pragma once
//======================================================================================================================
//  Includes
//----------------------------------------------------------------------------------------------------------------------
// Codec
#include "Codec/Audio/Frame.hpp"
#include "Codec/Audio/FrameResizer.hpp"
#include "Codec/Audio/Resampler.hpp"
// Core
#include "Strawberry/Core/IO/ChannelBroadcaster.hpp"
#include "Strawberry/Core/Util/Optional.hpp"
#include "Strawberry/Core/Util/Variant.hpp"
// Standard Library
#include <any>
#include <deque>
#include <functional>
#include <thread>
#include <vector>
#include <filesystem>


//======================================================================================================================
//  Class Declaration
//----------------------------------------------------------------------------------------------------------------------
namespace Strawberry::Codec::Audio
{
	class Playlist
	{
	public:
		struct SongBeganEvent {
			/// The index of the new currently playing song
			size_t   index;
			/// The difference in playlist index.
			int      offset;
			/// The data associated with the new song
			std::any associatedData;
		};


		struct SongAddedEvent {
			/// The index where the song was inserted.
			size_t   index;
			/// The data associated with the new song
			std::any associatedData;
		};


		struct SongRemovedEvent {
			/// The index where the song was inserted.
			size_t index;
		};


		struct PlaybackEndedEvent {};


		using Event         = Core::Variant<SongBeganEvent, SongAddedEvent, SongRemovedEvent, PlaybackEndedEvent>;

		using EventReceiver = std::shared_ptr<Core::IO::ChannelReceiver<Playlist::Event>>;


	public:
		Playlist(const Audio::FrameFormat& format, size_t sampleCount);
		Playlist(const Playlist& rhs)            = delete;
		Playlist& operator=(const Playlist& rhs) = delete;
		Playlist(Playlist&& rhs)                 = delete;
		Playlist& operator=(Playlist&& rhs)      = delete;
		~Playlist();


		Core::Optional<Frame> ReadFrame();


		[[nodiscard]] Core::Optional<size_t> EnqueueFile(const std::filesystem::path& path, const std::any& associatedData = {});


		void RemoveTrack(size_t index);


		EventReceiver CreateEventReceiver();


		[[nodiscard]] size_t                    GetCurrentTrackIndex() const;
		[[nodiscard]] size_t                    Length() const;
		[[nodiscard]] Codec::Audio::FrameFormat GetFrameFormat() const;
		[[nodiscard]] size_t                    GetFrameSize() const;


		template <typename T>
		[[nodiscard]] T GetTrackAssociatedData(size_t index) const;
		template <typename T>
		void SetTrackAssociatedData(size_t index, T value);


		void GotoPrevTrack();
		void GotoNextTrack();


	private:
		using FrameBuffer = std::vector<Frame>;
		using TrackLoader = std::function<void(Core::Mutex<FrameBuffer>&)>;


	private:
		void StartLoading(const TrackLoader& loader);
		void StopLoading(bool clearFrames);


	private:
		struct Track {
			TrackLoader loader;
			std::any    associatedData;
		};


		const Audio::FrameFormat mFormat;
		const size_t             mFrameSize;

		Resampler    mResampler;
		FrameResizer mFrameResizer;

		std::uint64_t            mCurrentPosition = 0;
		std::deque<Track>        mPreviousTracks;
		Core::Optional<Track>    mCurrentTrack;
		Core::Mutex<FrameBuffer> mCurrentTrackFrames;
		std::deque<Track>        mNextTracks;


		std::atomic<bool>         mShouldRead = false;
		Core::Optional<std::thread> mReadingThread;


		Core::IO::ChannelBroadcaster<Playlist::Event> mEventBroadcaster;
		bool                                          mHasSentPlaybackEnded = false;
	};


	template <typename T>
	T Playlist::GetTrackAssociatedData(size_t index) const
	{
		if (index < mPreviousTracks.size()) { return std::any_cast<T>(mPreviousTracks[index].associatedData); }
		else if (index == mPreviousTracks.size() && mCurrentTrack) { return std::any_cast<T>(mCurrentTrack->associatedData); }
		else if (index > mPreviousTracks.size()) { return std::any_cast<T>(mNextTracks[index - mPreviousTracks.size()].associatedData); }
		else { Core::Unreachable(); }
	}


	template <typename T>
	void Playlist::SetTrackAssociatedData(size_t index, T value)
	{
		if (index < mPreviousTracks.size()) { mPreviousTracks[index].associatedData = value; }
		else if (index == mPreviousTracks.size() && mCurrentTrack) { mCurrentTrack->associatedData = value; }
		else if (index == mPreviousTracks.size() && !mCurrentTrack && !mNextTracks.empty()) { mNextTracks.front().associatedData = value; }
		else if (index > mPreviousTracks.size())
		{
			if (mCurrentTrack) { mNextTracks[index - mPreviousTracks.size()].associatedData = value; }
			else { mNextTracks[index - mPreviousTracks.size() - 1].associatedData = value; }
		}
		else { Core::Unreachable(); }
	}
} // namespace Strawberry::Codec::Audio
