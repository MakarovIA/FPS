// By Polyakov Pavel

#pragma once

class UVoronoiRenderingComponent;

/** Voronoi mesh vertex */
struct PRACTICEPROJECT_API FVoronoiMeshVertex
{
    FVector Position;
    FColor Color;

    FPackedNormal TangentX;
    FPackedNormal TangentZ;

    FVector2D TextureCoordinate;

    FORCEINLINE FVoronoiMeshVertex(const FVector& InPosition, const FColor& InColor)
		: Position(InPosition), Color(InColor), TangentX(FVector(1, 0, 0)), TangentZ(FVector(0, 0, 1)), TextureCoordinate(0, 0)
    {
        TangentZ.Vector.W = 255;
    }
};

/** Vertex Buffer */
struct PRACTICEPROJECT_API FVoronoiVertexBuffer : public FVertexBuffer
{
    TArray<FVoronoiMeshVertex> Vertices;
    virtual void InitRHI() override;
};

/** Index Buffer */
struct PRACTICEPROJECT_API FVoronoiIndexBuffer : public FIndexBuffer
{
    TArray<int32> Indices;
    virtual void InitRHI() override;
};

/** Vertex Factory */
struct PRACTICEPROJECT_API FVoronoiVertexFactory : public FLocalVertexFactory
{
    void Init(const FVoronoiVertexBuffer* VertexBuffer);
};

/** Proxy data */
struct PRACTICEPROJECT_API FVoronoiSceneProxyData
{
    struct FLine
    {
        FVector Begin, End;
        FColor Color;
        float Width;

        FORCEINLINE FLine(const FVector& InBegin, const FVector& InEnd, const FColor& InColor, float InWidth = 0.f)
			: Begin(InBegin), End(InEnd), Color(InColor), Width(InWidth) {}
    };

    TArray<FVoronoiMeshVertex> Vertices;
    TArray<int32> Indices;

    TArray<FLine> Lines, Arcs;
};

/** Proxy for Voronoi rendering component */
class PRACTICEPROJECT_API FVoronoiSceneProxy : public FPrimitiveSceneProxy
{
    TArray<FVoronoiSceneProxyData::FLine> Lines, Arcs;

    FVoronoiIndexBuffer IndexBuffer;
    FVoronoiVertexBuffer VertexBuffer;
    FVoronoiVertexFactory VertexFactory;

    FMeshBatchElement MeshBatchElement;
    UMaterialInterface* VoronoiMaterial;

public:
    FVoronoiSceneProxy(UVoronoiRenderingComponent* InComponent, FVoronoiSceneProxyData ProxyData);
    virtual ~FVoronoiSceneProxy() override;

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

    virtual uint32 GetMemoryFootprint() const override { return sizeof(FVoronoiSceneProxy) + GetAllocatedSize(); }
    uint32 GetAllocatedSize() const { return FPrimitiveSceneProxy::GetAllocatedSize(); }
};
