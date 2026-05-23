#define PLUG_NAME "Industrial Iron"
#define PLUG_MFR "AwesomeJohnny"
#define PLUG_VERSION_HEX 0x00010000
#define PLUG_VERSION_STR "1.0.0"
// Unique IDs — do NOT share with other plugins
#define PLUG_UNIQUE_ID 'P3LS'
#define PLUG_MFR_ID 'wuwJ'
#define PLUG_URL_STR "https://example.com"
#define PLUG_EMAIL_STR "contact@example.com"
#define PLUG_COPYRIGHT_STR "Copyright 2026 AwesomeJohnny"
#define PLUG_CLASS_NAME IndustrialIron

#define BUNDLE_NAME "IndustrialIron"
#define BUNDLE_MFR "AwesomeJohnny"
#define BUNDLE_DOMAIN "com"

#define SHARED_RESOURCES_SUBPATH "IndustrialIron"

// Mono input -> Stereo output (IronWallPanner generates L/R from mono)
#define PLUG_CHANNEL_IO "1-2"

#define PLUG_LATENCY 0
#define PLUG_TYPE 0
#define PLUG_DOES_MIDI_IN 0
#define PLUG_DOES_MIDI_OUT 0
#define PLUG_DOES_MPE 0
#define PLUG_DOES_STATE_CHUNKS 0
#define PLUG_HAS_UI 1

// UI dimensions per SPEC section 7.1
#define PLUG_WIDTH 800
#define PLUG_HEIGHT 300
#define PLUG_FPS 60
#define PLUG_SHARED_RESOURCES 0
#define PLUG_HOST_RESIZE 0

#define AUV2_ENTRY IndustrialIron_Entry
#define AUV2_ENTRY_STR "IndustrialIron_Entry"
#define AUV2_FACTORY IndustrialIron_Factory
#define AUV2_VIEW_CLASS IndustrialIron_View
#define AUV2_VIEW_CLASS_STR "IndustrialIron_View"

#define AAX_TYPE_IDS 'IIF1'
#define AAX_TYPE_IDS_AUDIOSUITE 'IIA1'
#define AAX_PLUG_MFR_STR "AwesomeJohnny"
#define AAX_PLUG_NAME_STR "Industrial Iron\nIIFX"
#define AAX_PLUG_CATEGORY_STR "Effect"
#define AAX_DOES_AUDIOSUITE 0

#define VST3_SUBCATEGORY "Fx|Distortion"

// Resource filename macros
#define ROBOTO_FN "Roboto-Regular.ttf"

#define APP_NUM_CHANNELS 2
#define APP_N_VECTOR_WAIT 0
#define APP_MULT 1
#define APP_COPY_AUV3 0
#define APP_SIGNAL_VECTOR_SIZE 64
