#include "Codec/OpusEncoder.hpp"



#include "Codec/Packet.hpp"
#include "Core/Assert.hpp"



extern "C"
{
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
}



namespace Strawberry::Codec
{
	OpusEncoder::OpusEncoder()
		: mContext(nullptr)
		, mParameters(nullptr)
	{
		const AVCodec* opusCodec = avcodec_find_encoder(AV_CODEC_ID_OPUS);
		Core::Assert(opusCodec != nullptr);
		Core::Assert(av_codec_is_encoder(opusCodec));
		mContext = avcodec_alloc_context3(opusCodec);
		Core::Assert(mContext != nullptr);
		mContext->strict_std_compliance = -2;
		mContext->sample_rate = opusCodec->supported_samplerates[0];
		mContext->time_base   = AVRational{1, mContext->sample_rate};
		mContext->sample_fmt  = opusCodec->sample_fmts[0];
		mContext->ch_layout   = AV_CHANNEL_LAYOUT_STEREO;

		auto result = avcodec_open2(mContext, opusCodec, nullptr);
		Core::Assert(result == 0);
		Core::Assert(avcodec_is_open(mContext));

		mParameters = avcodec_parameters_alloc();
		Core::Assert(mParameters != nullptr);
		result = avcodec_parameters_from_context(mParameters, mContext);
		Core::Assert(result >= 0);

		mFrameResizer.Emplace(mContext->frame_size);
	}



	OpusEncoder::~OpusEncoder()
	{
		avcodec_parameters_free(&mParameters);
		avcodec_close(mContext);
		avcodec_free_context(&mContext);
	}



	std::vector<Packet> OpusEncoder::Encode(const Frame& frame)
	{
		std::vector<Packet> packets;

		auto frames = mFrameResizer->Process(frame);

		for (auto& resizedFrame : frames)
		{
			auto send = avcodec_send_frame(mContext, *resizedFrame);
			Core::Assert(send == 0);

			int receive;
			do
			{
				Packet packet;
				receive = avcodec_receive_packet(mContext, *packet);
				Core::Assert(receive == 0 || receive == AVERROR(EAGAIN));

				if (receive == 0)
				{
					packets.push_back(packet);
				}
			}
			while (receive == 0);
		}

		return packets;
	}



	AVCodecParameters* OpusEncoder::Parameters() const
	{
		return mParameters;
	}
}