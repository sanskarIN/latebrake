#pragma once

// UTF-8 portable policy for the separate Windows 2D trial's settings.dat.
// V3 and V4 share the UIB file family (low 24 magic bits). A newer record
// must remain read-only even if its full shape no longer matches V4.
// Avoid CRT/Windows dependencies: the native game links /NODEFAULTLIB.
namespace latebrake_preview
{
inline constexpr unsigned int PreviewSettingsMagicV3=0x33424955u;
inline constexpr unsigned int PreviewSettingsMagicV4=0x34424955u;
inline constexpr unsigned int PreviewSettingsCurrentVersion=4u;
constexpr bool IsNewerPreviewSettingsSchema(
    unsigned int magic,unsigned int version) noexcept
{
    return (magic&0x00FFFFFFu)==0x00424955u &&
        version>PreviewSettingsCurrentVersion;
}

struct PreviewSettingsState final
{
    int Car=0;
    int Track=0;
    int Event=0;
    bool Contrast=false;
    bool Assist=true;
};

// The on-disk write is authoritative: a failed rename/flush (or newer
// schema) must leave *all* UI settings at their previous in-memory values.
constexpr PreviewSettingsState ResolvePreviewSettingsSave(
    const PreviewSettingsState& previous,
    const PreviewSettingsState& desired,bool persisted) noexcept
{
    return persisted?desired:previous;
}

static_assert(!IsNewerPreviewSettingsSchema(PreviewSettingsMagicV3,3u));
static_assert(!IsNewerPreviewSettingsSchema(PreviewSettingsMagicV4,4u));
static_assert(IsNewerPreviewSettingsSchema(0x35424955u,5u));
static_assert(!IsNewerPreviewSettingsSchema(0xDEADBEEFu,0xFFFFFFFFu));
} // namespace latebrake_preview
