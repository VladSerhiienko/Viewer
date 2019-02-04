#pragma once
#include <cstdint>

namespace apemode {
namespace platform {

class AppInput;
struct AppSurface;

struct IAppShellCommandArgumentValue {
    enum ValueType {
        kValueType_StringValue,
        kValueType_UInt64Value,
        kValueType_UInt32Value,
        kValueType_UInt16Value,
        kValueType_UInt8Value,
        kValueType_Int64Value,
        kValueType_Int32Value,
        kValueType_Int16Value,
        kValueType_Int8Value,
        kValueType_Float64Value,
        kValueType_Float32Value,
        kValueType_PtrValue,
        kValueType_Array,
    };

    virtual ~IAppShellCommandArgumentValue( ) = default;

    virtual ValueType                            GetType( ) const                           = 0;
    virtual void*                                GetPtrValue( ) const                       = 0;
    virtual const char*                          GetStringValue( ) const                    = 0;
    virtual uint64_t                             GetUInt64Value( ) const                    = 0;
    virtual uint32_t                             GetUInt32Value( ) const                    = 0;
    virtual uint16_t                             GetUInt16Value( ) const                    = 0;
    virtual uint8_t                              GetUInt8Value( ) const                     = 0;
    virtual int64_t                              GetInt64Value( ) const                     = 0;
    virtual int32_t                              GetInt32Value( ) const                     = 0;
    virtual int16_t                              GetInt16Value( ) const                     = 0;
    virtual int8_t                               GetInt8Value( ) const                      = 0;
    virtual double                               GetFloat64Value( ) const                   = 0;
    virtual float                                GetFloat32Value( ) const                   = 0;
    virtual uint32_t                             GetArraySize( ) const                      = 0;
    virtual const IAppShellCommandArgumentValue* GetArrayValue( uint32_t valueIndex ) const = 0;
};

struct IAppShellCommandArgument {
    virtual ~IAppShellCommandArgument( ) = default;

    virtual const char*                          GetName( ) const = 0;
    virtual const IAppShellCommandArgumentValue* GetValue( ) const = 0;
};

struct IAppShellCommand {
    virtual ~IAppShellCommand( ) = default;

    virtual const char*                     GetType( ) const                                = 0;
    virtual int                             GetArgumentCount( ) const                       = 0;
    virtual const IAppShellCommandArgument* GetArgument( int argumentIndex ) const          = 0;
    virtual const IAppShellCommandArgument* FindArgumentByName( const char* pszName ) const = 0;
};

struct AppShellCommandResult {
    bool bSucceeded = true;
};

struct IAppShell {
    virtual ~IAppShell( ) = default;

    virtual AppShellCommandResult Execute( const IAppShellCommand* pCmd ) = 0;
};

} // namespace platform
} // namespace apemode
