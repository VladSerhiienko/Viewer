#pragma once

/* This file can be used outside the other framework with
 * TInfoStructHasStructType.Vulkan.inl
 * TInfoStructResolveStructType.Vulkan.inl
 */

namespace apemodevk
{
    /* Utilities to help to resolve the sType member in structs.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    namespace traits
    {
        template < typename TVulkanStructure >
        struct ResolveHasInfoStructureTypeFor {
            static const bool bHas = false;
        };

        template < typename TVulkanStructure >
        struct ResolveInfoStructureTypeFor {
            static const VkStructureType eType = VK_STRUCTURE_TYPE_MAX_ENUM;
        };
    }

    namespace traits
    {
        #ifdef _Define_has_struct_type
        #undef _Define_has_struct_type
        #endif

        #define _Define_has_struct_type(T)                                                                 \
            template <>                                                                                    \
            struct ResolveHasInfoStructureTypeFor<T>                                                                           \
            {                                                                                              \
                static const bool bHas = true;                                                             \
            }

        _Define_has_struct_type(VkCommandBufferBeginInfo);
        _Define_has_struct_type(VkRenderPassBeginInfo);
        _Define_has_struct_type(VkBufferMemoryBarrier);
        _Define_has_struct_type(VkImageMemoryBarrier);
        _Define_has_struct_type(VkFenceCreateInfo);
        _Define_has_struct_type(VkSemaphoreCreateInfo);
        _Define_has_struct_type(VkSubmitInfo);
        _Define_has_struct_type(VkPresentInfoKHR);
        _Define_has_struct_type(VkSwapchainCreateInfoKHR);
        _Define_has_struct_type(VkImageViewCreateInfo);
        _Define_has_struct_type(VkImageCreateInfo);
        _Define_has_struct_type(VkMemoryAllocateInfo);
        _Define_has_struct_type(VkSamplerCreateInfo);
        _Define_has_struct_type(VkBufferCreateInfo);
        _Define_has_struct_type(VkDescriptorSetLayoutCreateInfo);
        _Define_has_struct_type(VkRenderPassCreateInfo);
        _Define_has_struct_type(VkShaderModuleCreateInfo);
        _Define_has_struct_type(VkPipelineDynamicStateCreateInfo);
        _Define_has_struct_type(VkGraphicsPipelineCreateInfo);
        _Define_has_struct_type(VkPipelineVertexInputStateCreateInfo);
        _Define_has_struct_type(VkPipelineInputAssemblyStateCreateInfo);
        _Define_has_struct_type(VkPipelineRasterizationStateCreateInfo);
        _Define_has_struct_type(VkPipelineColorBlendStateCreateInfo);
        _Define_has_struct_type(VkPipelineDepthStencilStateCreateInfo);
        _Define_has_struct_type(VkPipelineViewportStateCreateInfo);
        _Define_has_struct_type(VkPipelineMultisampleStateCreateInfo);
        _Define_has_struct_type(VkPipelineShaderStageCreateInfo);
        _Define_has_struct_type(VkPipelineLayoutCreateInfo);
        _Define_has_struct_type(VkPipelineCacheCreateInfo);
        _Define_has_struct_type(VkDescriptorPoolCreateInfo);
        _Define_has_struct_type(VkDescriptorSetAllocateInfo);
        _Define_has_struct_type(VkWriteDescriptorSet);
        _Define_has_struct_type(VkFramebufferCreateInfo);
        _Define_has_struct_type(VkCommandPoolCreateInfo);
        _Define_has_struct_type(VkCommandBufferAllocateInfo);
        _Define_has_struct_type(VkApplicationInfo);
        _Define_has_struct_type(VkInstanceCreateInfo);
        _Define_has_struct_type(VkDebugReportCallbackCreateInfoEXT);
        _Define_has_struct_type(VkDeviceQueueCreateInfo);
        _Define_has_struct_type(VkDeviceCreateInfo);
        _Define_has_struct_type(VkCommandBufferInheritanceInfo);
        _Define_has_struct_type(VkMappedMemoryRange);

        #if defined(VK_USE_PLATFORM_WIN32_KHR) && VK_USE_PLATFORM_WIN32_KHR == 1
        _Define_has_struct_type(VkWin32SurfaceCreateInfoKHR);
        #endif

        #if defined(VK_USE_PLATFORM_XLIB_KHR) && VK_USE_PLATFORM_XLIB_KHR == 1
        _Define_has_struct_type(VkXlibSurfaceCreateInfoKHR);
        #endif

        #undef _Define_has_struct_type
    }

    namespace traits
    {
        #ifdef _Define_resolve_struct_type
        #undef _Define_resolve_struct_type
        #endif

        #define _Define_resolve_struct_type(T, E)       \
            template <>                                 \
            struct ResolveInfoStructureTypeFor<T>                        \
            {                                           \
                static const VkStructureType eType = E; \
            }

        _Define_resolve_struct_type(VkCommandBufferBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO);
        _Define_resolve_struct_type(VkRenderPassBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO);
        _Define_resolve_struct_type(VkMemoryBarrier, VK_STRUCTURE_TYPE_MEMORY_BARRIER);
        _Define_resolve_struct_type(VkBufferMemoryBarrier, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER);
        _Define_resolve_struct_type(VkImageMemoryBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER);
        _Define_resolve_struct_type(VkFenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO);
        _Define_resolve_struct_type(VkSemaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO);
        _Define_resolve_struct_type(VkSubmitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO);
        _Define_resolve_struct_type(VkPresentInfoKHR, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR);
        _Define_resolve_struct_type(VkSwapchainCreateInfoKHR, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR);
        _Define_resolve_struct_type(VkImageViewCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO);
        _Define_resolve_struct_type(VkImageCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO);
        _Define_resolve_struct_type(VkMemoryAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO);
        _Define_resolve_struct_type(VkSamplerCreateInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO);
        _Define_resolve_struct_type(VkBufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO);
        _Define_resolve_struct_type(VkDescriptorSetLayoutCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO);
        _Define_resolve_struct_type(VkRenderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO);
        _Define_resolve_struct_type(VkShaderModuleCreateInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineDynamicStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkGraphicsPipelineCreateInfo, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineVertexInputStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineInputAssemblyStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineRasterizationStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineColorBlendStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineDepthStencilStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineViewportStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineMultisampleStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO);
        _Define_resolve_struct_type(VkPipelineShaderStageCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO);
        _Define_resolve_struct_type(VkDescriptorPoolCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO);
        _Define_resolve_struct_type(VkDescriptorSetAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO);
        _Define_resolve_struct_type(VkWriteDescriptorSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET);
        _Define_resolve_struct_type(VkFramebufferCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO);
        _Define_resolve_struct_type(VkCommandPoolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO);
        _Define_resolve_struct_type(VkCommandBufferAllocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO);
        _Define_resolve_struct_type(VkPipelineLayoutCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO);
        _Define_resolve_struct_type(VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO);
        _Define_resolve_struct_type(VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO);
        _Define_resolve_struct_type(VkDebugReportCallbackCreateInfoEXT, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT);
        _Define_resolve_struct_type(VkDeviceQueueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO);
        _Define_resolve_struct_type(VkDeviceCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO);

        #if defined(VK_USE_PLATFORM_WIN32_KHR) && VK_USE_PLATFORM_WIN32_KHR == 1
        _Define_resolve_struct_type(VkWin32SurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR);
        #endif

        #if defined(VK_USE_PLATFORM_XLIB_KHR) && VK_USE_PLATFORM_XLIB_KHR == 1
        _Define_resolve_struct_type(VkXlibSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR);
        #endif

        _Define_resolve_struct_type(VkCommandBufferInheritanceInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO);
        _Define_resolve_struct_type(VkMappedMemoryRange, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE);

        #undef _Define_resolve_struct_type
    }

    namespace traits {

        template < typename TVulkanStructure >
        inline bool IsValid( TVulkanStructure const &Desc ) {
            static const VkStructureType UnresolvedType = VK_STRUCTURE_TYPE_MAX_ENUM;
            using HasTypeField = ResolveHasInfoStructureTypeFor< TVulkanStructure >;

            const auto sType = *reinterpret_cast< VkStructureType const * >( &Desc );
            const bool bMustHaveValidType = HasTypeField::bHas;
            const bool bValid = ( !bMustHaveValidType ) || ( bMustHaveValidType && ( sType != UnresolvedType ) );
            return bValid;
        }
    }

    /* Utility structure that wraps the native Vulkan structures and sets sType if applicable
     * Since it is quite time consuming to substitute all the usages in the exising code
     * There are global functions implemented below the class to address the issue
     */
    template <typename TVulkanNativeStruct>
    class TInfoStruct
    {
    public:
        static const bool HasStructType = traits::ResolveHasInfoStructureTypeFor< TVulkanNativeStruct >::bHas;

        typedef TVulkanNativeStruct VulkanNativeStructType;
        typedef TInfoStruct< TVulkanNativeStruct > SelfType;
        typedef std::vector< SelfType > VectorImpl; /* TODO: Remove */

        /* TODO: Remove */
        struct Vector : public VectorImpl {
            static_assert( sizeof( SelfType ) == sizeof( TVulkanNativeStruct ), "Size mismatch." );
            inline TVulkanNativeStruct *data( ) {
                return reinterpret_cast< TVulkanNativeStruct * >( VectorImpl::data( ) );
            }

            inline TVulkanNativeStruct const *data( ) const {
                return reinterpret_cast< TVulkanNativeStruct const * >( VectorImpl::data( ) );
            }
        };

        TVulkanNativeStruct Desc;

        inline TInfoStruct( ) { InitializeStruct( ); }
        inline TInfoStruct( SelfType &&Other ) : Desc( Other.Desc ) { }
        inline TInfoStruct( SelfType const &Other ) : Desc( Other.Desc ) { }
        inline TInfoStruct( TVulkanNativeStruct const &OtherDesc ) : Desc( OtherDesc ) { SetStructType( ); }

        inline SelfType & operator =(SelfType && Other) { Desc = Other.Desc; Validate(); return *this; }
        inline SelfType & operator =(TVulkanNativeStruct && OtherDesc) { Desc = OtherDesc; Validate(); return *this; }
        inline SelfType & operator =(SelfType const & Other) { Desc = Other.Desc; Validate(); return *this; }
        inline SelfType & operator =(TVulkanNativeStruct const & OtherDesc) { Desc = OtherDesc; Validate();  return *this; }

        inline TVulkanNativeStruct * operator->() { return &Desc; }
        inline TVulkanNativeStruct const * operator->() const { return &Desc; }
        inline operator TVulkanNativeStruct &() { Validate(); return Desc; }
        inline operator TVulkanNativeStruct *() { Validate(); return &Desc; }
        inline operator TVulkanNativeStruct const &() const { Validate(); return Desc; }
        inline operator TVulkanNativeStruct const *() const { Validate(); return &Desc; }

        inline void InitializeStruct( ) {
            InitializeStruct( Desc );
        }

        inline void Validate( ) const {
            Validate( Desc );
        }

        static void Validate( const TVulkanNativeStruct &NativeDesc ) {
            assert( traits::IsValid( NativeDesc ) && "Desc is corrupted." );
        }

        static void SetStructType( TVulkanNativeStruct &NativeDesc ) {
            if ( HasStructType ) {
                auto sType = reinterpret_cast< VkStructureType * >( &NativeDesc );
                *sType = traits::ResolveInfoStructureTypeFor<TVulkanNativeStruct>::eType;
            }
        }

        template < size_t _ArraySize >
        static void SetStructType( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
            if ( HasStructType ) {
                auto sType = reinterpret_cast< VkStructureType * >( &NativeDescs[ 0 ] );
                for ( auto &NativeDesc : NativeDescs ) {
                    *sType = traits::ResolveInfoStructureTypeFor< TVulkanNativeStruct >::eType;
                    sType = reinterpret_cast<VkStructureType*>((reinterpret_cast<uint8_t*>(sType) + sizeof( TVulkanNativeStruct )));
                }
            }
        }

        static void SetStructType( TVulkanNativeStruct * pNativeDescs, size_t pNativeDescCount ) {
            if ( HasStructType ) {
                auto sType = reinterpret_cast< VkStructureType * >( pNativeDescs );
                for ( size_t i = 0; i < pNativeDescCount; ++i ) {
                    *sType = traits::ResolveInfoStructureTypeFor< TVulkanNativeStruct >::eType;
                    sType += sizeof( TVulkanNativeStruct );
                }
            }
        }

        static void InitializeStruct( TVulkanNativeStruct &Desc ) {
            ZeroMemory( Desc );
            SetStructType( Desc );
        }

        template < size_t _ArraySize >
        static void InitializeStruct( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
            ZeroMemory( NativeDescs );
            SetStructType( NativeDescs );
        }

        static void InitializeStruct( TVulkanNativeStruct *pNativeDescs, size_t pNativeDescCount ) {
            ZeroMemory( pNativeDescs, pNativeDescCount );
            SetStructType( pNativeDescs, pNativeDescCount );
        }
    };

    ///
    /// Utility structures, that make it easier to integrate
    /// the Vulkan wrapper into the existing code bases.
    ///

    /* Sets sType structure member if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void SetStructType( TVulkanNativeStruct &NativeDesc ) {
        TInfoStruct< TVulkanNativeStruct >::SetStructType( NativeDesc );
    }

    /* Sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct, size_t _ArraySize >
    inline void SetStructType( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
        TInfoStruct< TVulkanNativeStruct >::SetStructType( NativeDescs );
    }

    /* Sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void SetStructType( TVulkanNativeStruct *pNativeDescs, size_t pNativeDescCount ) {
        TInfoStruct< TVulkanNativeStruct >::SetStructType( pNativeDescs, pNativeDescCount );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void InitializeStruct( TVulkanNativeStruct &NativeDesc ) {
        TInfoStruct< TVulkanNativeStruct >::InitializeStruct( NativeDesc );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct, size_t _ArraySize >
    inline void InitializeStruct( TVulkanNativeStruct ( &NativeDescs )[ _ArraySize ] ) {
        TInfoStruct< TVulkanNativeStruct >::InitializeStruct( NativeDescs );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * All the structures are listed in TInfoStructHasStructType.Vulkan.inl
     * All the sType values are defined in TInfoStructResolveStructType.Vulkan.inl
     */
    template < typename TVulkanNativeStruct >
    inline void InitializeStruct( TVulkanNativeStruct *pNativeDescs, size_t pNativeDescCount ) {
        TInfoStruct< TVulkanNativeStruct >::InitializeStruct( pNativeDescs, pNativeDescCount );
    }

    /* Please, see InitializeStruct( ) */
    template < typename TVulkanNativeStruct >
    TVulkanNativeStruct NewInitializedStruct( ) {
        TVulkanNativeStruct NativeDesc;
        InitializeStruct( NativeDesc );
        return NativeDesc;
    }

} // namespace apemodevk

namespace apemodevk
{
    template <typename T>
    inline uint32_t GetSizeU(T const & c) {
        return _Get_collection_length_u(c);
    }

    /** Aliasing cares only about size matching. */
    template < typename TVector, typename TNativeDesc >
    static void AliasStructs( TVector const &Descs, TNativeDesc const *&pOutDescs, uint32_t &OutDescCount ) {
        using ValueType = typename TVector::value_type;
        static_assert( sizeof( ValueType ) == sizeof( TNativeDesc ), "Size mismatch, cannot alias." );
        pOutDescs = reinterpret_cast< TNativeDesc const * >( Descs.data( ) );
        OutDescCount = GetSizeU( Descs );
    }

    /** Aliasing cares both about type and size matching.
     * @see AliasStructs for the lighter version of the function.
     */
    template < typename TInfoStructVector, typename TNativeDesc >
    static void AliasInfoStructs( TInfoStructVector const &Descs,
                                  TNativeDesc const *&     pOutAliasedDescsRef,
                                  uint32_t &               OutDescCount ) {
        using InfoStructValueType = typename TInfoStructVector::value_type;
        using NativeDescValueType = typename InfoStructValueType::VulkanNativeStructType;
        static_assert( std::is_same< TNativeDesc, NativeDescValueType >::value, "Types are not same, cannot alias." );
        AliasStructs( Descs, pOutAliasedDescsRef, OutDescCount );
    }
} // namespace apemodevk
