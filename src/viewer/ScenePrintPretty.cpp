
#include "Scene.h"

#include <apemode/platform/AppState.h>
#include <apemode/platform/memory/MemoryManager.h>

namespace {
void PrintPrettyPush( char c, char *depth, int &di ) {
    depth[ di++ ] = ' ';
    depth[ di++ ] = c;
    depth[ di++ ] = ' ';
    depth[ di++ ] = ' ';
    depth[ di ]   = 0;
}

void PrintPrettyPop( char *depth, int &di ) {
    depth[ di -= 4 ] = 0;
}

void PrintPrettyPrint( const apemode::Scene &    scene,
                       const apemode::SceneNode &sceneNode,
                       char *                    pszDepthString,
                       int &                     depthStringIndex ) {

    auto childRange = scene.NodeToChildIds.equal_range( sceneNode.Id );
    while ( childRange.first != childRange.second ) {
        const bool bHasNext = eastl::next( childRange.first ) != childRange.second;

        const apemode::SceneNode &childSceneNode = scene.Nodes[ childRange.first->second ];
        apemode::LogInfo( "{} {} {} {} [ID={}]",
                          pszDepthString,
                          bHasNext ? "|--" : "`--",
                          childSceneNode.pszName,
                          childSceneNode.eSkeletonType != apemode::detail::eSkeletonType_None ? "(J)" : "",
                          childSceneNode.Id );

        PrintPrettyPush( bHasNext ? '|' : ' ', pszDepthString, depthStringIndex );
        PrintPrettyPrint( scene, childSceneNode, pszDepthString, depthStringIndex );
        PrintPrettyPop( pszDepthString, depthStringIndex );

        ++childRange.first;
    }
}

void PrintPrettyPrint( const apemode::Scene &scene ) {

    const apemode::SceneNode &sceneNode = scene.Nodes.front( );
    apemode::LogInfo( " {} {} [ID={}]",
                      sceneNode.pszName,
                      sceneNode.eSkeletonType != apemode::detail::eSkeletonType_None ? "(J)" : "",
                      sceneNode.Id );

    int  depthIndex = 0;
    char depth[ 2056 ] = {0};
    PrintPrettyPrint( scene, sceneNode, depth, depthIndex );
}
} // namespace

void apemode::detail::ScenePrintPretty::PrintPretty( const apemode::Scene *pScene ) {
    if ( pScene )
        PrintPrettyPrint( *pScene );
}
