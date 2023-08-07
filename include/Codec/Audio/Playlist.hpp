#pragma once
//======================================================================================================================
//  Includes
//----------------------------------------------------------------------------------------------------------------------
// Codec
#include "Codec/Audio/Frame.hpp"
#include "Codec/Audio/Resampler.hpp"
#include "Codec/Audio/FrameResizer.hpp"
// Core
#include "Strawberry/Core/Util/Option.hpp"
#include "Strawberry/Core/Sync/Mutex.hpp"
// Standard Library
#include <deque>
#include <functional>


//======================================================================================================================
//  Class Declaration
//----------------------------------------------------------------------------------------------------------------------
namespace Strawberry::Codec::Audio
{
	class Playlist
	{
	public:
		Playlist(Audio::FrameFormat format, size_t sampleCount);


		Core::Option<Frame>            ReadFrame();


		void                           EnqueueFile(const std::string path);


	private:
		const Audio::FrameFormat                        mFormat;
		const size_t                                    mFrameSize;

		Resampler                                       mResampler;
		FrameResizer                                    mFrameResizer;

		Core::Mutex<std::deque<Frame>>                               mFrames;
		Core::Mutex<std::deque<std::function<std::vector<Frame>()>>> mNextTracks;
	};
}
