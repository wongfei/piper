#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>
#include <spdlog/spdlog.h>

#include "json.hpp"
#include "piper.hpp"
#include "piper_wrapper.h"

using namespace std;
using json = nlohmann::json;
using piper_buffer_type = std::vector<int16_t>;

struct piper_context
{
	piper::PiperConfig config;
	piper::Voice voice;
};

static std::string last_error; // M$ STYLE!

extern "C" {

PIPER_API const char* piper_get_error()
{
	return last_error.c_str();
}

PIPER_API piper_context* piper_init(const char* model_path, const char* data_path, bool use_cuda)
{
	last_error = "";
	piper_context* context = nullptr;

	try
	{
		spdlog::set_level(spdlog::level::warn);

		context = new piper_context();
		context->config.eSpeakDataPath = std::string(data_path);
		
		std::string modelPath(model_path);
		std::string modelConfigPath(modelPath + ".json");
		std::optional<piper::SpeakerId> speakerId; //((piper::SpeakerId)speaker_id);

		piper::loadVoice(context->config, modelPath, modelConfigPath, context->voice, speakerId, use_cuda);
		piper::initialize(context->config);
	}
	catch (const std::exception& ex)
	{
		last_error = ex.what();
		if (context) { delete context; context = nullptr; }
	}
	
	return context;
}

PIPER_API void piper_release(piper_context* context)
{
	last_error = "";
	
	if (context)
	{
		try
		{
			piper::terminate(context->config);
		}
		catch (const std::exception& ex)
		{
			last_error = ex.what();
		}
		delete context;
	}
}

PIPER_API int piper_get_voice_sample_rate(piper_context* context)
{
	return context ? context->voice.synthesisConfig.sampleRate : 0;
}

PIPER_API int piper_get_voice_sample_bytes(piper_context* context)
{
	return context ? context->voice.synthesisConfig.sampleWidth : 0;
}

PIPER_API int piper_get_voice_channels(piper_context* context)
{
	return context ? context->voice.synthesisConfig.channels : 0;
}

PIPER_API int piper_get_num_speakers(piper_context* context)
{
	return context ? context->voice.modelConfig.numSpeakers : 0;
}

PIPER_API piper_buffer_ptr piper_alloc_buffer()
{
	auto buf = new piper_buffer_type();
	return (piper_buffer_ptr)buf;
}

PIPER_API void piper_free_buffer(piper_buffer_ptr bufp)
{
	auto buf = (piper_buffer_type*)bufp;
	delete buf;
}

PIPER_API void* piper_get_buffer_data(piper_buffer_ptr bufp)
{
	auto buf = (piper_buffer_type*)bufp;
	return buf->data();
}

PIPER_API size_t piper_get_buffer_size(piper_buffer_ptr bufp)
{
	auto buf = (piper_buffer_type*)bufp;
	return buf->size() * sizeof(buf->data()[0]);
}

PIPER_API int piper_text_to_buffer(piper_context* context, const char* text, piper_buffer_ptr bufp, int speaker_id, volatile int* pcancel_flag)
{
	last_error = "";
	try
	{
		context->voice.synthesisConfig.speakerId = speaker_id;
		auto buf = (piper_buffer_type*)bufp;

		piper::SynthesisResult res;
		res.pcancel_flag = pcancel_flag;
		
		piper::textToAudio(context->config, context->voice, std::string(text), *buf, res, {});
		return 0;
	}
	catch (const std::exception& ex)
	{
		last_error = ex.what();
	}
	return -1;
}

PIPER_API int piper_text_to_file(piper_context* context, const char* text, const char* filename, int speaker_id)
{
	last_error = "";
	try
	{
		context->voice.synthesisConfig.speakerId = speaker_id;

		ofstream audioFile(filename, ios::binary);
		piper::SynthesisResult res;

		piper::textToWavFile(context->config, context->voice, std::string(text), audioFile, res);
		return 0;
	}
	catch (const std::exception& ex)
	{
		last_error = ex.what();
	}
	return -1;
}

#define PIPER_BIND_FP(name) api->name##_fp = &name

PIPER_FUNC (int, piper_get_api, (struct piper_api* api))
{
	if (!api) return -1;

	PIPER_BIND_FP(piper_get_error);

	PIPER_BIND_FP(piper_init);
	PIPER_BIND_FP(piper_release);

	PIPER_BIND_FP(piper_get_voice_sample_rate);
	PIPER_BIND_FP(piper_get_voice_sample_bytes);
	PIPER_BIND_FP(piper_get_voice_channels);
	PIPER_BIND_FP(piper_get_num_speakers);

	PIPER_BIND_FP(piper_alloc_buffer);
	PIPER_BIND_FP(piper_free_buffer);
	PIPER_BIND_FP(piper_get_buffer_data);
	PIPER_BIND_FP(piper_get_buffer_size);

	PIPER_BIND_FP(piper_text_to_buffer);
	PIPER_BIND_FP(piper_text_to_file);

	return 0;
}

} // extern "C"
