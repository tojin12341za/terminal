// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "GlobalAppSettings.h"
#include "../../types/inc/Utils.hpp"
#include "../../inc/DefaultSettings.h"
#include "Utils.h"
#include "JsonUtils.h"
#include <sstream>

using namespace TerminalApp;
using namespace winrt::Microsoft::Terminal::Settings;
using namespace winrt::TerminalApp;
using namespace winrt::Windows::Data::Json;
using namespace winrt::Windows::UI::Xaml;
using namespace ::Microsoft::Console;
using namespace winrt::Microsoft::UI::Xaml::Controls;

static constexpr std::string_view KeybindingsKey{ "keybindings" };
static constexpr std::string_view DefaultProfileKey{ "defaultProfile" };
static constexpr std::string_view AlwaysShowTabsKey{ "alwaysShowTabs" };
static constexpr std::string_view InitialRowsKey{ "initialRows" };
static constexpr std::string_view InitialColsKey{ "initialCols" };
static constexpr std::string_view RowsToScrollKey{ "rowsToScroll" };
static constexpr std::string_view InitialPositionKey{ "initialPosition" };
static constexpr std::string_view ShowTitleInTitlebarKey{ "showTerminalTitleInTitlebar" };
static constexpr std::string_view ThemeKey{ "theme" };
static constexpr std::string_view TabWidthModeKey{ "tabWidthMode" };
static constexpr std::string_view EqualTabWidthModeValue{ "equal" };
static constexpr std::string_view TitleLengthTabWidthModeValue{ "titleLength" };
static constexpr std::string_view ShowTabsInTitlebarKey{ "showTabsInTitlebar" };
static constexpr std::string_view WordDelimitersKey{ "wordDelimiters" };
static constexpr std::string_view CopyOnSelectKey{ "copyOnSelect" };
static constexpr std::string_view CopyFormattingKey{ "copyFormatting" };
static constexpr std::string_view LaunchModeKey{ "launchMode" };
static constexpr std::string_view ConfirmCloseAllKey{ "confirmCloseAllTabs" };
static constexpr std::string_view SnapToGridOnResizeKey{ "snapToGridOnResize" };
static constexpr std::string_view DefaultLaunchModeValue{ "default" };
static constexpr std::string_view MaximizedLaunchModeValue{ "maximized" };
static constexpr std::string_view LightThemeValue{ "light" };
static constexpr std::string_view DarkThemeValue{ "dark" };
static constexpr std::string_view SystemThemeValue{ "system" };

static constexpr std::string_view DebugFeaturesKey{ "debugFeatures" };

#ifdef _DEBUG
static constexpr bool debugFeaturesDefault{ true };
#else
static constexpr bool debugFeaturesDefault{ false };
#endif

template<>
struct JsonUtils::ConversionTrait<ElementTheme> : public JsonUtils::KeyValueMapper<ElementTheme, JsonUtils::ConversionTrait<ElementTheme>>
{
    static constexpr std::array<pair_type, 3> mappings = {
        pair_type{ SystemThemeValue, ElementTheme::Default },
        pair_type{ LightThemeValue, ElementTheme::Light },
        pair_type{ DarkThemeValue, ElementTheme::Dark },
    };
};

template<>
struct JsonUtils::ConversionTrait<LaunchMode> : public JsonUtils::KeyValueMapper<LaunchMode, JsonUtils::ConversionTrait<LaunchMode>>
{
    static constexpr std::array<pair_type, 2> mappings = {
        pair_type{ DefaultLaunchModeValue, LaunchMode::DefaultMode },
        pair_type{ MaximizedLaunchModeValue, LaunchMode::MaximizedMode },
    };
};

template<>
struct JsonUtils::ConversionTrait<TabViewWidthMode> : public JsonUtils::KeyValueMapper<TabViewWidthMode, JsonUtils::ConversionTrait<TabViewWidthMode>>
{
    static constexpr std::array<pair_type, 2> mappings = {
        pair_type{ EqualTabWidthModeValue, TabViewWidthMode::Equal },
        pair_type{ TitleLengthTabWidthModeValue, TabViewWidthMode::SizeToContent },
    };
};

GlobalAppSettings::GlobalAppSettings() :
    _keybindings{ winrt::make_self<winrt::TerminalApp::implementation::AppKeyBindings>() },
    _keybindingsWarnings{},
    _colorSchemes{},
    _InitialRows{ DEFAULT_ROWS },
    _InitialCols{ DEFAULT_COLS },
    _RowsToScroll{ DEFAULT_ROWSTOSCROLL },
    _WordDelimiters{ DEFAULT_WORD_DELIMITERS },
    _DebugFeaturesEnabled{ debugFeaturesDefault }
{
}

GlobalAppSettings::~GlobalAppSettings()
{
}

std::unordered_map<std::wstring, ColorScheme>& GlobalAppSettings::GetColorSchemes() noexcept
{
    return _colorSchemes;
}

const std::unordered_map<std::wstring, ColorScheme>& GlobalAppSettings::GetColorSchemes() const noexcept
{
    return _colorSchemes;
}

AppKeyBindings GlobalAppSettings::GetKeybindings() const noexcept
{
    return *_keybindings;
}

// Method Description:
// - Applies appropriate settings from the globals into the given TerminalSettings.
// Arguments:
// - settings: a TerminalSettings object to add global property values to.
// Return Value:
// - <none>
void GlobalAppSettings::ApplyToSettings(TerminalSettings& settings) const noexcept
{
    settings.KeyBindings(GetKeybindings());
    settings.InitialRows(_InitialRows);
    settings.InitialCols(_InitialCols);
    settings.RowsToScroll(_RowsToScroll);

    settings.WordDelimiters(_WordDelimiters);
    settings.CopyOnSelect(_CopyOnSelect);
}

// Method Description:
// - Serialize this object to a JsonObject.
// Arguments:
// - <none>
// Return Value:
// - a JsonObject which is an equivalent serialization of this object.
Json::Value GlobalAppSettings::ToJson() const
{
    return Json::Value::nullSingleton();
}

// Method Description:
// - Create a new instance of this class from a serialized JsonObject.
// Arguments:
// - json: an object which should be a serialization of a GlobalAppSettings object.
// Return Value:
// - a new GlobalAppSettings instance created from the values in `json`
GlobalAppSettings GlobalAppSettings::FromJson(const Json::Value& json)
{
    GlobalAppSettings result;
    result.LayerJson(json);
    return result;
}

void GlobalAppSettings::LayerJson(const Json::Value& json)
{
    JsonUtils::GetValueForKey(json, DefaultProfileKey, _DefaultProfile);

    JsonUtils::GetValueForKey(json, AlwaysShowTabsKey, _AlwaysShowTabs);

    JsonUtils::GetValueForKey(json, ConfirmCloseAllKey, _ConfirmCloseAllTabs);

    JsonUtils::GetValueForKey(json, InitialRowsKey, _InitialRows);

    JsonUtils::GetValueForKey(json, InitialColsKey, _InitialCols);

    // TODO GH#XXXX: manual parsing!
    if (auto rowsToScroll{ json[JsonKey(RowsToScrollKey)] })
    {
        //if it's not an int we fall back to setting it to 0, which implies using the system setting. This will be the case if it's set to "system"
        if (rowsToScroll.isInt())
        {
            _RowsToScroll = rowsToScroll.asInt();
        }
        else
        {
            _RowsToScroll = 0;
        }
    }

    JsonUtils::GetValueForKey(json, InitialPositionKey, _InitialPosition);

    JsonUtils::GetValueForKey(json, ShowTitleInTitlebarKey, _ShowTitleInTitlebar);

    JsonUtils::GetValueForKey(json, ShowTabsInTitlebarKey, _ShowTabsInTitlebar);

    JsonUtils::GetValueForKey(json, WordDelimitersKey, _WordDelimiters);

    JsonUtils::GetValueForKey(json, CopyOnSelectKey, _CopyOnSelect);

    JsonUtils::GetValueForKey(json, CopyFormattingKey, _CopyFormatting);

    JsonUtils::GetValueForKey(json, LaunchModeKey, _LaunchMode);

    JsonUtils::GetValueForKey(json, ThemeKey, _Theme);

    JsonUtils::GetValueForKey(json, TabWidthModeKey, _TabWidthMode);

    JsonUtils::GetValueForKey(json, SnapToGridOnResizeKey, _SnapToGridOnResize);

    // GetValueForKey will only override the current value if the key exists
    JsonUtils::GetValueForKey(json, DebugFeaturesKey, _DebugFeaturesEnabled);

    if (auto keybindings{ json[JsonKey(KeybindingsKey)] })
    {
        auto warnings = _keybindings->LayerJson(keybindings);
        // It's possible that the user provided keybindings have some warnings
        // in them - problems that we should alert the user to, but we can
        // recover from. Most of these warnings cannot be detected later in the
        // Validate settings phase, so we'll collect them now. If there were any
        // warnings generated from parsing these keybindings, add them to our
        // list of warnings.
        _keybindingsWarnings.insert(_keybindingsWarnings.end(), warnings.begin(), warnings.end());
    }
}

// Method Description:
// - Helper function for converting the initial position string into
//   2 coordinate values. We allow users to only provide one coordinate,
//   thus, we use comma as the separator:
//   (100, 100): standard input string
//   (, 100), (100, ): if a value is missing, we set this value as a default
//   (,): both x and y are set to default
//   (abc, 100): if a value is not valid, we treat it as default
//   (100, 100, 100): we only read the first two values, this is equivalent to (100, 100)
// Arguments:
// - initialPosition: the initial position string from json
//   initialX: reference to the _initialX member
//   initialY: reference to the _initialY member
// Return Value:
// - None
template<>
struct JsonUtils::ConversionTrait<LaunchPosition>
{
    LaunchPosition FromJson(const Json::Value& json)
    {
        LaunchPosition ret;
        std::string initialPosition{ json.asString() };
        static constexpr char singleCharDelim = ',';
        std::stringstream tokenStream(initialPosition);
        std::string token;
        uint8_t initialPosIndex = 0;

        // Get initial position values till we run out of delimiter separated values in the stream
        // or we hit max number of allowable values (= 2)
        // Non-numeral values or empty string will be caught as exception and we do not assign them
        for (; std::getline(tokenStream, token, singleCharDelim) && (initialPosIndex < 2); initialPosIndex++)
        {
            try
            {
                int32_t position = std::stoi(token);
                if (initialPosIndex == 0)
                {
                    ret.x.emplace(position);
                }

                if (initialPosIndex == 1)
                {
                    ret.y.emplace(position);
                }
            }
            catch (...)
            {
                // Do nothing
            }
        }
        return ret;
    }

    bool CanConvert(const Json::Value& json)
    {
        return json.isString();
    }
};

// Method Description:
// - Adds the given colorscheme to our map of schemes, using its name as the key.
// Arguments:
// - scheme: the color scheme to add
// Return Value:
// - <none>
void GlobalAppSettings::AddColorScheme(ColorScheme scheme)
{
    std::wstring name{ scheme.GetName() };
    _colorSchemes[name] = std::move(scheme);
}

// Method Description:
// - Return the warnings that we've collected during parsing the JSON for the
//   keybindings. It's possible that the user provided keybindings have some
//   warnings in them - problems that we should alert the user to, but we can
//   recover from.
// Arguments:
// - <none>
// Return Value:
// - <none>
std::vector<TerminalApp::SettingsLoadWarnings> GlobalAppSettings::GetKeybindingsWarnings() const
{
    return _keybindingsWarnings;
}
