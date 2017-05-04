// By Polyakov Pavel

#include "PracticeProject.h"
#include "VoronoiRenderingComponent.h"
#include "VoronoiSceneProxy.h"

void FVoronoiVertexBuffer::InitRHI()
{
    FRHIResourceCreateInfo CreateInfo;
    VertexBufferRHI = RHICreateVertexBuffer(Vertices.Num() * sizeof(FVoronoiMeshVertex), BUF_Static, CreateInfo);
    void* VertexBufferData = RHILockVertexBuffer(VertexBufferRHI, 0, Vertices.Num() * sizeof(FVoronoiMeshVertex), RLM_WriteOnly);
    FMemory::Memcpy(VertexBufferData, Vertices.GetData(), Vertices.Num() * sizeof(FVoronoiMeshVertex));
    RHIUnlockVertexBuffer(VertexBufferRHI);
}

void FVoronoiIndexBuffer::InitRHI()
{
    FRHIResourceCreateInfo CreateInfo;
    IndexBufferRHI = RHICreateIndexBuffer(sizeof(int32), Indices.Num() * sizeof(int32), BUF_Static, CreateInfo);
    void* Buffer = RHILockIndexBuffer(IndexBufferRHI, 0, Indices.Num() * sizeof(int32), RLM_WriteOnly);
    FMemory::Memcpy(Buffer, Indices.GetData(), Indices.Num() * sizeof(int32));
    RHIUnlockIndexBuffer(IndexBufferRHI);
}

void FVoronoiVertexFactory::Init(const FVoronoiVertexBuffer* VertexBuffer)
{
    ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
        InitVoronoiVertexFactory,
        FVoronoiVertexFactory*, VertexFactory, this,
        const FVoronoiVertexBuffer*, VertexBuffer, VertexBuffer,
        {
            DataType NewData;
            NewData.PositionComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FVoronoiMeshVertex, Position, VET_Float3);
            NewData.TangentBasisComponents[0] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FVoronoiMeshVertex, TangentX, VET_PackedNormal);
            NewData.TangentBasisComponents[1] = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FVoronoiMeshVertex, TangentZ, VET_PackedNormal);
            NewData.ColorComponent = STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FVoronoiMeshVertex, Color, VET_Color);
            NewData.TextureCoordinates.Add(STRUCTMEMBER_VERTEXSTREAMCOMPONENT(VertexBuffer, FVoronoiMeshVertex, TextureCoordinate, VET_Float2));

            VertexFactory->SetData(NewData);
        });
}

/** FVoronoiSceneProxy */

FVoronoiSceneProxy::FVoronoiSceneProxy(UVoronoiRenderingComponent* InComponent, FVoronoiSceneProxyData ProxyData)
    : FPrimitiveSceneProxy(InComponent)
{
    Lines = MoveTemp(ProxyData.Lines);
    Arcs = MoveTemp(ProxyData.Arcs);

    VertexBuffer.Vertices = MoveTemp(ProxyData.Vertices);
    IndexBuffer.Indices = MoveTemp(ProxyData.Indices);

    MeshBatchElement.FirstIndex = 0;
    MeshBatchElement.MinVertexIndex = 0;
    MeshBatchElement.MaxVertexIndex = VertexBuffer.Vertices.Num();
    MeshBatchElement.IndexBuffer = &IndexBuffer;
    MeshBatchElement.NumPrimitives = FMath::FloorToInt(IndexBuffer.Indices.Num() / 3);

    if (MeshBatchElement.NumPrimitives > 0)
    {
        VertexFactory.Init(&VertexBuffer);

        BeginInitResource(&IndexBuffer);
        BeginInitResource(&VertexBuffer);
        BeginInitResource(&VertexFactory);
    }

    VoronoiMaterial = static_cast<AVoronoiNavData*>(InComponent->GetOwner())->GetDrawingOptions().VoronoiMaterial;
}

FVoronoiSceneProxy::~FVoronoiSceneProxy()
{
    VertexBuffer.ReleaseResource();
    IndexBuffer.ReleaseResource();
    VertexFactory.ReleaseResource();
}

void FVoronoiSceneProxy::GetDynamicMeshElements(const TArray<const FSceneView*>& Views, const FSceneViewFamily& ViewFamily, uint32 VisibilityMap, FMeshElementCollector& Collector) const
{
    for (int32 ViewIndex = 0, ViewsNum = Views.Num(); ViewIndex < ViewsNum; ViewIndex++)
    {
        if (VisibilityMap & (1 << ViewIndex))
        {
            if (FPrimitiveDrawInterface *PDI = Collector.GetPDI(ViewIndex))
            {
                for (const FVoronoiSceneProxyData::FLine& Line : Lines)
                    PDI->DrawLine(Line.Begin, Line.End, Line.Color, SDPG_World, Line.Width);

                for (const FVoronoiSceneProxyData::FLine& Arc : Arcs)
                    DrawArc(PDI, Arc.Begin, Arc.End, 0.3f, 16, Arc.Color, SDPG_World, Arc.Width);
            }

            if (MeshBatchElement.NumPrimitives == 0 || VoronoiMaterial == nullptr)
                continue;

            FMeshBatch& Mesh = Collector.AllocateMesh();
            Mesh.Elements[0] = MeshBatchElement;
            Mesh.Elements[0].PrimitiveUniformBuffer = CreatePrimitiveUniformBufferImmediate(FMatrix::Identity, GetBounds(), GetLocalBounds(), false, UseEditorDepthTest());

            Mesh.bWireframe = false;
            Mesh.VertexFactory = &VertexFactory;
            Mesh.MaterialRenderProxy = VoronoiMaterial ? VoronoiMaterial->GetRenderProxy(false) : nullptr;
            Mesh.ReverseCulling = IsLocalToWorldDeterminantNegative();
            Mesh.Type = PT_TriangleList;
            Mesh.DepthPriorityGroup = SDPG_World;
            Mesh.bCanApplyViewModeOverrides = false;
            Mesh.CastShadow = false;

            Collector.AddMesh(ViewIndex, Mesh);
        }
    }
}

FPrimitiveViewRelevance FVoronoiSceneProxy::GetViewRelevance(const FSceneView* View) const
{
    FPrimitiveViewRelevance Result;
    Result.bSeparateTranslucencyRelevance = VoronoiMaterial != nullptr;
    Result.bNormalTranslucencyRelevance = VoronoiMaterial && VoronoiMaterial->GetBlendMode() == EBlendMode::BLEND_Translucent;
    Result.bDynamicRelevance = true;
    Result.bDrawRelevance = true;

    return Result;
}
