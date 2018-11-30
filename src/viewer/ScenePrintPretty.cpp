
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

void PrintPrettyPrint( const apemode::Scene *pScene, const apemode::SceneNode *pNode, char *depth, int &di ) {
    printf( " %s %s [ID=%u]\n",
            pNode->pszName,
            pNode->eSkeletonType != apemode::detail::eSkeletonType_None ? "(J)" : "",
            pNode->Id );

    auto childRange = pScene->NodeToChildIds.equal_range( pNode->Id );
    while ( childRange.first != childRange.second ) {
        bool bHasNext = eastl::next( childRange.first ) != childRange.second;
        if ( bHasNext ) {
            printf( "%s ├──", depth );
            PrintPrettyPush( '|', depth, di );
        } else {
            printf( "%s └──", depth );
            PrintPrettyPush( ' ', depth, di );
        }

        PrintPrettyPrint( pScene, &pScene->Nodes[ childRange.first->second ], depth, di );
        PrintPrettyPop( depth, di );
        ++childRange.first;
    }
}
}

void apemode::detail::ScenePrintPretty::PrintPretty( const apemode::Scene *pScene ) {
    int  depthIndex    = 0;
    char depth[ 2056 ] = {0};
    PrintPrettyPrint( pScene, &pScene->Nodes.front( ), depth, depthIndex );
}
