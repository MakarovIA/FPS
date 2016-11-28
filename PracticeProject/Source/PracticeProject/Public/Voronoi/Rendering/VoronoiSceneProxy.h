// By Polyakov Pavel

#pragma once

class UVoronoiRenderingComponent;

/** Voronoi mesh vertex */
struct FVoronoiMeshVertex
{
    FVector Position;
    FColor Color;

    FPackedNormal TangentX;
    FPackedNormal TangentZ;

    FVector2D TextureCoordinate;

    FVoronoiMeshVertex(const FVector& InPosition, const FColor& InColor) : Position(InPosition), Color(InColor), TangentX(FVector(1, 0, 0)), TangentZ(FVector(0, 0, 1)), TextureCoordinate(0, 0)
    {
        TangentZ.Vector.W = 255;
    }
};

/** Vertex Buffer */
class FVoronoiVertexBuffer : public FVertexBuffer
{
public:
    TArray<FVoronoiMeshVertex> Vertices;
    virtual void InitRHI() override;
};

/** Index Buffer */
class FVoronoiIndexBuffer : public FIndexBuffer
{
public:
    TArray<int32> Indices;
    virtual void InitRHI() override;
};

/** Vertex Factory */
class FVoronoiVertexFactory : public FLocalVertexFactory
{
public:
    void Init(const FVoronoiVertexBuffer* VertexBuffer);
};

/** Proxy data */
struct FVoronoiSceneProxyData
{
    struct FLine
    {
        FVector Begin, End;
        FColor Color;

        FLine(const FVector& InBegin, const FVector& InEnd, const FColor& InColor) : Begin(InBegin), End(InEnd), Color(InColor) {}
    };

    TArray<FVoronoiMeshVertex> Vertices;
    TArray<int32> Indices;

    TArray<FLine> Lines;
};

/** Proxy for Voronoi rendering component */
class FVoronoiSceneProxy : public FPrimitiveSceneProxy
{
    TArray<FVoronoiSceneProxyData::FLine> Lines;

    FVoronoiIndexBuffer IndexBuffer;
    FVoronoiVertexBuffer VertexBuffer;
    FVoronoiVertexFactory VertexFactory;

    FMeshBatchElement MeshBatchElement;
    UMaterial* VoronoiMaterial;

public:
    FVoronoiSceneProxy(UVoronoiRenderingComponent* InComponent, FVoronoiSceneProxyData &ProxyData);
    virtual ~FVoronoiSceneProxy() override;

    virtual void GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const override;
    virtual FPrimitiveViewRelevance GetViewRelevance(const FSceneView* View) const override;

    virtual uint32 GetMemoryFootprint(void) const override { return sizeof(FVoronoiSceneProxy) + GetAllocatedSize(); }
    uint32 GetAllocatedSize(void) const { return FPrimitiveSceneProxy::GetAllocatedSize(); }
};
