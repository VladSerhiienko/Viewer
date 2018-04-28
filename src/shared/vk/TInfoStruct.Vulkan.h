#pragma once

#include <type_traits>

namespace apemodevk
{
    namespace traits {

        /* Returns VkStructureType for the template argument.
         * Returns VK_STRUCTURE_TYPE_MAX_ENUM if it failed to relate a template argument with the listed structure.
         */
        template< typename TStruct>
        inline VkStructureType GetStructureType( ) {
#pragma region Relating structures with their struct type.
#define APEMODEVK_MAP_TYPE_TO_STRUCT( T, V ) \
    if ( std::is_same< TStruct, T >::value ) \
        return V;

            APEMODEVK_MAP_TYPE_TO_STRUCT( VkCommandBufferBeginInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkRenderPassBeginInfo, VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkMemoryBarrier, VK_STRUCTURE_TYPE_MEMORY_BARRIER );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkBufferMemoryBarrier, VK_STRUCTURE_TYPE_BUFFER_MEMORY_BARRIER );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkImageMemoryBarrier, VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkFenceCreateInfo, VK_STRUCTURE_TYPE_FENCE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkSemaphoreCreateInfo, VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkSubmitInfo, VK_STRUCTURE_TYPE_SUBMIT_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPresentInfoKHR, VK_STRUCTURE_TYPE_PRESENT_INFO_KHR );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkSwapchainCreateInfoKHR, VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkImageViewCreateInfo, VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkImageCreateInfo, VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkMemoryAllocateInfo, VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkSamplerCreateInfo, VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkBufferCreateInfo, VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkDescriptorSetLayoutCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkRenderPassCreateInfo, VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkShaderModuleCreateInfo, VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineDynamicStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkGraphicsPipelineCreateInfo, VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineCacheCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_CACHE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineVertexInputStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineInputAssemblyStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineRasterizationStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineColorBlendStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineDepthStencilStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineViewportStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineMultisampleStateCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineShaderStageCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkDescriptorPoolCreateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkDescriptorSetAllocateInfo, VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkWriteDescriptorSet, VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkFramebufferCreateInfo, VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkCommandPoolCreateInfo, VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkCommandBufferAllocateInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkPipelineLayoutCreateInfo, VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkApplicationInfo, VK_STRUCTURE_TYPE_APPLICATION_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkInstanceCreateInfo, VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkDebugReportCallbackCreateInfoEXT, VK_STRUCTURE_TYPE_DEBUG_REPORT_CREATE_INFO_EXT );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkDeviceQueueCreateInfo, VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkDeviceCreateInfo, VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkCommandBufferInheritanceInfo, VK_STRUCTURE_TYPE_COMMAND_BUFFER_INHERITANCE_INFO );
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkMappedMemoryRange, VK_STRUCTURE_TYPE_MAPPED_MEMORY_RANGE );

#if defined( VK_USE_PLATFORM_WIN32_KHR ) && VK_USE_PLATFORM_WIN32_KHR == 1
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkWin32SurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR );
#endif
#if defined( VK_USE_PLATFORM_XLIB_KHR ) && VK_USE_PLATFORM_XLIB_KHR == 1
            APEMODEVK_MAP_TYPE_TO_STRUCT( VkXlibSurfaceCreateInfoKHR, VK_STRUCTURE_TYPE_XLIB_SURFACE_CREATE_INFO_KHR );
#endif
            return VK_STRUCTURE_TYPE_MAX_ENUM;
#pragma endregion
        }

        /* Returns true if there is a valid VkStructureType value for the template argument.
         * Returns false otherwise.
         */
        template < typename TStruct >
        inline bool HasStructureType( ) {
            return GetStructureType< TStruct >( ) != VK_STRUCTURE_TYPE_MAX_ENUM;
        }
    }

    /* Sets sType structure member if applicable.
     * @see GetStructureType(), HasStructureType().
     * @note Map a missing type in GetStructureType().
     */
    template < typename TStruct >
    inline void SetStructType( TStruct &uninitializedStruct ) {
        using namespace traits;
        if ( HasStructureType< TStruct >( ) ) {
            auto sType = reinterpret_cast< VkStructureType * >( &uninitializedStruct );
            *sType     = GetStructureType< TStruct >( );
        }
    }

    /* Sets sType structure members if applicable.
     * @see GetStructureType(), HasStructureType().
     * @note Map a missing type in GetStructureType().
     */
    template < typename TStruct, size_t TArraySize >
    inline void SetStructType( TStruct ( &uninitializedStructs )[ TArraySize ] ) {
        using namespace traits;
        if ( HasStructureType< TStruct >( ) ) {
            auto sType = reinterpret_cast< VkStructureType * >( &uninitializedStructs[ 0 ] );
            for ( auto &uninitializedStruct : uninitializedStructs ) {
                *sType = GetStructureType< TStruct >( );
                sType = reinterpret_cast< VkStructureType * >( ( reinterpret_cast< uint8_t * >( sType ) + sizeof( TStruct ) ) );
            }
        }
    }

    /* Sets sType structure members if applicable.
     * @see GetStructureType(), HasStructureType().
     * @note Map a missing type in GetStructureType().
     */
    template < typename TStruct >
    inline void SetStructType( TStruct *pUninitializedStructs, size_t count ) {
        using namespace traits;
        if ( HasStructureType< TStruct >( ) ) {
            auto sType = reinterpret_cast< VkStructureType * >( pUninitializedStructs );
            for ( size_t i = 0; i < count; ++i ) {
                *sType = GetStructureType< TStruct >( );
                sType += sizeof( TStruct );
            }
        }
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * @see GetStructureType(), HasStructureType().
     * @note Map a missing type in GetStructureType().
     */
    template < typename TStruct >
    inline void InitializeStruct( TStruct &uninitializedStruct ) {
        ZeroMemory( uninitializedStruct );
        SetStructType( uninitializedStruct );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * @see GetStructureType(), HasStructureType().
     * @note Map a missing type in GetStructureType().
     */
    template < typename TStruct, size_t TArraySize >
    inline void InitializeStruct( TStruct ( &uninitializedStructs )[ TArraySize ] ) {
        ZeroMemory( uninitializedStructs );
        SetStructType( uninitializedStructs );
    }

    /* Zeros all the provided memory, and sets sType structure members if applicable.
     * @see GetStructureType(), HasStructureType().
     * @note Map a missing type in GetStructureType().
     */
    template < typename TStruct >
    inline void InitializeStruct( TStruct *pUninitializedStructs, size_t count ) {
            ZeroMemory( pUninitializedStructs, count );
            SetStructType( pUninitializedStructs, count );
    }

    /* Returns initialized structure (zeroed members, correct sType if applicable.
     * @see GetStructureType(), HasStructureType(), map a missing type there.
     * @note Map a missing type in GetStructureType().
     * @seealso InitializeStruct()
     */
    template < typename TStruct >
    TStruct TNewInitializedStruct( ) {
        TStruct uninitializedStruct;
        InitializeStruct( uninitializedStruct );
        return uninitializedStruct;
    }

} // namespace apemodevk
