#include "AppInput.h"

using namespace apemode::platform;

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