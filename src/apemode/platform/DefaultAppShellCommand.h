#pragma once

#include "IAppShell.h"

#include <string>
#include <map>
#include <vector>
#include <cstdint>

namespace apemode {
namespace platform {

struct AppShellCommandArgumentValue : IAppShellCommandArgumentValue {
    union ValueUnion {
        void*       PtrValue;
        const char* StringValue;
        uint64_t    UInt64Value;
        uint32_t    UInt32Value;
        uint16_t    UInt16Value;
        uint8_t     UInt8Value;
        int64_t     Int64Value;
        int32_t     Int32Value;
        int16_t     Int16Value;
        int8_t      Int8Value;
        double      Float64Value;
        float       Float32Value;
    };

    ValueType                                   Type;
    ValueUnion                                  Value;
    std::vector< AppShellCommandArgumentValue > ArrayValues;

    AppShellCommandArgumentValue( ) : Type( kValueType_PtrValue ) {
        Value.PtrValue = nullptr;
    }

    ValueType GetType( ) const override {
        return Type;
    }

#define APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( T ) \
    decltype( ValueUnion::T ) Get##T( ) const override { \
        assert( Type == kValueType_##T );                \
        return Value.T;                                  \
    }
#define APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( T ) \
    void Set##T( decltype( ValueUnion::T ) value ) {     \
        Type    = kValueType_##T;                        \
        Value.T = value;                                 \
        ArrayValues.clear( );                            \
    }

    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( PtrValue );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( StringValue );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( UInt64Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( UInt32Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( UInt16Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( UInt8Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( Int64Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( Int32Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( Int16Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( Int8Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( Float64Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER( Float32Value );

    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( PtrValue );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( StringValue );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( UInt64Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( UInt32Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( UInt16Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( UInt8Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( Int64Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( Int32Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( Int16Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( Int8Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( Float64Value );
    APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER( Float32Value );
    
#undef APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_GETTER
#undef APPSHELLCOMMANDARGVALUE_DEFINE_TYPED_SETTER

    uint32_t GetArraySize( ) const override {
        assert( Type == kValueType_Array );
        return uint32_t( ArrayValues.size( ) );
    }
    const IAppShellCommandArgumentValue* GetArrayValue( uint32_t valueIndex) const override {
        assert( Type == kValueType_Array );
        assert( valueIndex < uint32_t( ArrayValues.size( ) ) );
        return &ArrayValues[ valueIndex ];
    }

    void SetArray( ) {
        Type = kValueType_Array;
    }

    decltype( ArrayValues ) & GetArray( ) {
        assert( Type == kValueType_Array );
        return ArrayValues;
    }
};

struct AppShellCommandArgument : IAppShellCommandArgument {
    ~AppShellCommandArgument( ) = default;

    std::string                  Name;
    AppShellCommandArgumentValue Value;

    const char* GetName( ) const override {
        return Name.c_str( );
    }
    const IAppShellCommandArgumentValue* GetValue( ) const override {
        return &Value;
    }
};

struct AppShellCommand : IAppShellCommand {
    ~AppShellCommand( ) = default;

    std::string                                      Type;
    std::map< std::string, AppShellCommandArgument > Args;

    const char* GetType( ) const override {
        return Type.c_str( );
    }

    int GetArgumentCount( ) const override {
        return static_cast< int >( Type.size( ) );
    }

    const IAppShellCommandArgument* GetArgument( int argumentIndex ) const override {
        if ( argumentIndex >= 0 && argumentIndex < GetArgumentCount( ) ) {
            auto argumentIt = std::begin( Args );
            std::advance( argumentIt, (size_t) argumentIndex );
            return &argumentIt->second;
        }

        return nullptr;
    }

    const IAppShellCommandArgument* FindArgumentByName( const char* pszName ) const override {
        auto argumentIt = Args.find( pszName );
        if ( argumentIt != Args.end( ) ) {
            return &argumentIt->second;
        }

        return nullptr;
    }
};

} // namespace platform
} // namespace apemode
