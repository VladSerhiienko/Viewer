#pragma once

#include <stdint.h>
#include <assert.h>
#include <algorithm>
#include <type_traits>
#include <cstring>

namespace apemode {
namespace platform {

enum DigitalInput {
    // keyboard
    kDigitalInput_KeyEscape = 0,
    kDigitalInput_Key1,
    kDigitalInput_Key2,
    kDigitalInput_Key3,
    kDigitalInput_Key4,
    kDigitalInput_Key5,
    kDigitalInput_Key6,
    kDigitalInput_Key7,
    kDigitalInput_Key8,
    kDigitalInput_Key9,
    kDigitalInput_Key0,
    kDigitalInput_KeyMinus,
    kDigitalInput_KeyEquals,
    kDigitalInput_KeyBack,
    kDigitalInput_KeyTab,
    kDigitalInput_KeyQ,
    kDigitalInput_KeyW,
    kDigitalInput_KeyE,
    kDigitalInput_KeyR,
    kDigitalInput_KeyT,
    kDigitalInput_KeyY,
    kDigitalInput_KeyU,
    kDigitalInput_KeyI,
    kDigitalInput_KeyO,
    kDigitalInput_KeyP,
    kDigitalInput_KeyLeftBracket,
    kDigitalInput_KeyRightBracket,
    kDigitalInput_KeyReturn,
    kDigitalInput_KeyLeftControl,
    kDigitalInput_KeyA,
    kDigitalInput_KeyS,
    kDigitalInput_KeyD,
    kDigitalInput_KeyF,
    kDigitalInput_KeyG,
    kDigitalInput_KeyH,
    kDigitalInput_KeyJ,
    kDigitalInput_KeyK,
    kDigitalInput_KeyL,
    kDigitalInput_KeySemicolon,
    kDigitalInput_KeyApostrophe,
    kDigitalInput_KeyGrave,
    kDigitalInput_KeyLeftShift,
    kDigitalInput_KeyBackslash,
    kDigitalInput_KeyZ,
    kDigitalInput_KeyX,
    kDigitalInput_KeyC,
    kDigitalInput_KeyV,
    kDigitalInput_KeyB,
    kDigitalInput_KeyN,
    kDigitalInput_KeyM,
    kDigitalInput_KeyComma,
    kDigitalInput_KeyPeriod,
    kDigitalInput_KeySlash,
    kDigitalInput_KeyRightShift,
    kDigitalInput_KeyLeftAlt,
    kDigitalInput_KeySpace,
    kDigitalInput_KeyCapital,
    kDigitalInput_KeyF1,
    kDigitalInput_KeyF2,
    kDigitalInput_KeyF3,
    kDigitalInput_KeyF4,
    kDigitalInput_KeyF5,
    kDigitalInput_KeyF6,
    kDigitalInput_KeyF7,
    kDigitalInput_KeyF8,
    kDigitalInput_KeyF9,
    kDigitalInput_KeyF10,
    kDigitalInput_KeyF11,
    kDigitalInput_KeyF12,
    kDigitalInput_KeyRightControl,
    kDigitalInput_KeyDivide,
    kDigitalInput_KeyRightAlt,
    kDigitalInput_KeyPause,
    kDigitalInput_KeyHome,
    kDigitalInput_KeyUp,
    kDigitalInput_KeyLeft,
    kDigitalInput_KeyRight,
    kDigitalInput_KeyEnd,
    kDigitalInput_KeyDown,
    kDigitalInput_KeyInsert,
    kDigitalInput_KeyDelete,

    kDigitalInput_NumKeys,

    // gamepad
    kDigitalInput_DPadUp = kDigitalInput_NumKeys,
    kDigitalInput_DPadDown,
    kDigitalInput_DPadLeft,
    kDigitalInput_DPadRight,
    kDigitalInput_StartButton,
    kDigitalInput_BackButton,
    kDigitalInput_LeftThumbClick,
    kDigitalInput_RightThumbClick,
    kDigitalInput_LeftShoulder,
    kDigitalInput_RightShoulder,
    kDigitalInput_AButton,
    kDigitalInput_BButton,
    kDigitalInput_XButton,
    kDigitalInput_YButton,

    // mouse
    kDigitalInput_Mouse0,
    kDigitalInput_Mouse1,
    kDigitalInput_Mouse2,
    kDigitalInput_Mouse3,
    kDigitalInput_Mouse4,

    // mouse
    kDigitalInput_Touch0,
    kDigitalInput_TouchMaxCount = 16,

    kDigitalInput_NumInputs = kDigitalInput_Touch0 + kDigitalInput_TouchMaxCount,
};

enum AnalogInput {
    // gamepad
    kAnalogInput_LeftTrigger,
    kAnalogInput_RightTrigger,
    kAnalogInput_LeftStickX,
    kAnalogInput_LeftStickY,
    kAnalogInput_RightStickX,
    kAnalogInput_RightStickY,

    // mouse
    kAnalogInput_MouseX,
    kAnalogInput_MouseY,
    kAnalogInput_MouseHorzScroll,
    kAnalogInput_MouseVertScroll,
    kAnalogInput_Touch0X,
    kAnalogInput_Touch0Y = kAnalogInput_Touch0X + kDigitalInput_TouchMaxCount,

    kAnalogInput_NumInputs = kAnalogInput_Touch0X + 2 * kDigitalInput_TouchMaxCount,
};

using DigitalInputUInt = std::underlying_type< DigitalInput >::type;
using AnalogInputUInt  = std::underlying_type< AnalogInput >::type;

/* Contains input state information. Tracks touches, mouse and keyboard.
 * AnalogInput is anything that can be dragged/pulled (floats).
 * DigitalInput is anything and can be pressed (bools).
 * @note Should be extented to handle delta x and y for mouse/touches.
 * @note Should be extented to handle joystick.
 * @note Hold duration array still contains last pressed duration after
 *       the key was released. The value will be reset at the next press.
 */
class AppInput {
public:
    static uint32_t const sInvalidTouchValue = kDigitalInput_TouchMaxCount;

public:
    bool     bFocused;
    bool     bSizeChanged;
    bool     bIsUsingTouch;
    bool     bIsAnyPressed;
    bool     bIsQuitRequested;
    bool     bIsTrackingTouchesOrMousePressed;
    bool     Buttons[ 2 ][ kDigitalInput_NumInputs ];
    float    HoldDuration[ kDigitalInput_NumInputs ];
    float    Analogs[ kAnalogInput_NumInputs ];
    float    AnalogsTimeCorrected[ kAnalogInput_NumInputs ];
    uint32_t TouchIds[ kDigitalInput_TouchMaxCount ];
    uint32_t TouchIdCount;

public:
    inline AppInput( );
    inline ~AppInput( );

public:
    /* @return True if touches are supported and enabled.
     * @note False value means mouse is currently being tracked.
     */
    inline bool     IsTouchEnabled( ) const;
    inline uint32_t GetTouchCount( ) const;
    inline uint32_t GetFirstTouchId( ) const;
    inline uint32_t GetLastTouchId( ) const;
    inline bool     IsTouchTracked( uint32_t TouchId ) const;

    inline bool  IsAnyPressed( ) const;
    inline bool  IsTrackingTouchesOrMousePressed( ) const;
    inline bool  IsPressed( DigitalInputUInt InDigitalInput ) const;
    inline bool  IsFirstPressed( DigitalInputUInt InDigitalInput ) const;
    inline bool  IsReleased( DigitalInputUInt InDigitalInput ) const;
    inline bool  IsFirstReleased( DigitalInputUInt InDigitalInput ) const;
    inline float GetDurationPressed( DigitalInputUInt InDigitalInput ) const;
    inline float GetAnalogInput( AnalogInputUInt InAnalogInput ) const;
    inline float GetTimeCorrectedAnalogInput( AnalogInputUInt InAnalogInput ) const;
};

AppInput::AppInput( )
    : bFocused( false )
    , bSizeChanged( false )
    , bIsUsingTouch( false )
    , bIsAnyPressed( false )
    , bIsQuitRequested( false )
    , bIsTrackingTouchesOrMousePressed( false ) {
    memset( Buttons, 0, sizeof( Buttons ) );
    memset( HoldDuration, 0, sizeof( HoldDuration ) );
    memset( Analogs, 0, sizeof( Analogs ) );
    memset( AnalogsTimeCorrected, 0, sizeof( AnalogsTimeCorrected ) );

    TouchIdCount = 0;
    for ( auto& TouchId : TouchIds )
        TouchId = sInvalidTouchValue;
}

AppInput::~AppInput( ) {
}

bool AppInput::IsTouchEnabled( ) const {
    return bIsUsingTouch;
}

uint32_t AppInput::GetFirstTouchId( ) const {
    assert( TouchIdCount > 0 && "Invalid." );
    return TouchIds[ 0 ];
}

uint32_t AppInput::GetLastTouchId( ) const {
    assert( TouchIdCount > 0 && "Out of range." );
    return TouchIds[ TouchIdCount - 1 ];
}

bool AppInput::IsTouchTracked( uint32_t TouchId ) const {
    const auto TouchIt    = TouchIds;
    const auto TouchItEnd = TouchIds + TouchIdCount;
    return std::find( TouchIt, TouchItEnd, TouchId ) != TouchItEnd;
}

uint32_t AppInput::GetTouchCount( ) const {
    return TouchIdCount;
}

bool AppInput::IsAnyPressed( ) const {
    return bIsAnyPressed;
}

bool AppInput::IsTrackingTouchesOrMousePressed( ) const {
    return bIsTrackingTouchesOrMousePressed;
}

bool AppInput::IsPressed( DigitalInputUInt InDigitalInput ) const {
    return Buttons[ 0 ][ InDigitalInput ];
}

bool AppInput::IsFirstPressed( DigitalInputUInt InDigitalInput ) const {
    return Buttons[ 0 ][ InDigitalInput ] && !Buttons[ 1 ][ InDigitalInput ];
}

bool AppInput::IsReleased( DigitalInputUInt InDigitalInput ) const {
    return !Buttons[ 0 ][ InDigitalInput ];
}

bool AppInput::IsFirstReleased( DigitalInputUInt InDigitalInput ) const {
    return !Buttons[ 0 ][ InDigitalInput ] && Buttons[ 1 ][ InDigitalInput ];
}

float AppInput::GetDurationPressed( DigitalInputUInt InDigitalInput ) const {
    return HoldDuration[ InDigitalInput ];
}

float AppInput::GetAnalogInput( AnalogInputUInt InAnalogInput ) const {
    return Analogs[ InAnalogInput ];
}

} // namespace platform
} // namespace apemode
