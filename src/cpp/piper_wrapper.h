#include <stdint.h>

#ifdef PIPER_EXPORTS
	#define PIPER_API __declspec(dllexport)
#elif PIPER_IMPORTS
	#define PIPER_API __declspec(dllimport)
#else
	#define PIPER_API
#endif

struct piper_context;
typedef void* piper_buffer_ptr;

extern "C" {

PIPER_API const char* piper_get_error();

PIPER_API piper_context* piper_init(const char* model_path, const char* data_path, int speaker_id, bool use_cuda);
PIPER_API void piper_release(piper_context* context);

PIPER_API int piper_get_voice_sample_rate(piper_context* context);
PIPER_API int piper_get_voice_sample_bytes(piper_context* context);
PIPER_API int piper_get_voice_channels(piper_context* context);

PIPER_API piper_buffer_ptr piper_alloc_buffer();
PIPER_API void piper_free_buffer(piper_buffer_ptr bufp);
PIPER_API void* piper_get_buffer_data(piper_buffer_ptr bufp);
PIPER_API size_t piper_get_buffer_size(piper_buffer_ptr bufp);

PIPER_API int piper_text_to_buffer(piper_context* context, const char* text, piper_buffer_ptr bufp);
PIPER_API int piper_text_to_file(piper_context* context, const char* text, const char* filename);

}
