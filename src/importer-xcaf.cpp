#include "importer-xcaf.hpp"
#include "importer-utils.hpp"

#include <thread>
#include <vector>
#include <mutex>

#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Face.hxx>
#include <TDF_ChildIterator.hxx>
#include <TDocStd_Document.hxx>
#include <TDataStd_Name.hxx>
#include <Quantity_Color.hxx>
#include <BRep_Tool.hxx>
#include <XCAFDoc_DocumentTool.hxx>

static std::string GetLabelNameNoRef (const TDF_Label& label)
{
    Handle (TDataStd_Name) nameAttribute = new TDataStd_Name ();
    if (!label.FindAttribute (nameAttribute->GetID (), nameAttribute)) {
        return std::string ();
    }

    Standard_Integer utf8NameLength = nameAttribute->Get ().LengthOfCString ();
    char* nameBuf = new char[utf8NameLength + 1];
    nameAttribute->Get ().ToUTF8CString (nameBuf);
    std::string name (nameBuf, utf8NameLength);
    delete[] nameBuf;
    return name;
}

static std::string GetLabelName (const TDF_Label& label, const Handle (XCAFDoc_ShapeTool)& shapeTool)
{
    if (XCAFDoc_ShapeTool::IsReference (label)) {
        TDF_Label referredShapeLabel;
        shapeTool->GetReferredShape (label, referredShapeLabel);
        return GetLabelName (referredShapeLabel, shapeTool);
    }
    return GetLabelNameNoRef (label);
}

static std::string GetShapeName (const TopoDS_Shape& shape, const Handle (XCAFDoc_ShapeTool)& shapeTool)
{
    TDF_Label shapeLabel;
    if (!shapeTool->Search (shape, shapeLabel)) {
        return std::string ();
    }
    return GetLabelName (shapeLabel, shapeTool);
}

static bool GetLabelColorNoRef (const TDF_Label& label, const Handle (XCAFDoc_ColorTool)& colorTool, Color& color)
{
    static const std::vector<XCAFDoc_ColorType> colorTypes = {
        XCAFDoc_ColorSurf,
        XCAFDoc_ColorCurv,
        XCAFDoc_ColorGen
    };

    Quantity_Color qColor;
    for (XCAFDoc_ColorType colorType : colorTypes) {
        if (colorTool->GetColor (label, colorType, qColor)) {
            color = Color (qColor.Red (), qColor.Green (), qColor.Blue ());
            return true;
        }
    }

    return false;
}

static bool GetLabelColor (const TDF_Label& label, const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool, Color& color)
{
    if (GetLabelColorNoRef (label, colorTool, color)) {
        return true;
    }

    if (XCAFDoc_ShapeTool::IsReference (label)) {
        TDF_Label referredShape;
        shapeTool->GetReferredShape (label, referredShape);
        return GetLabelColor (referredShape, shapeTool, colorTool, color);
    }

    return false;
}

static bool GetShapeColor (const TopoDS_Shape& shape, const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool, Color& color)
{
    TDF_Label shapeLabel;
    if (!shapeTool->Search (shape, shapeLabel)) {
        return false;
    }
    return GetLabelColor (shapeLabel, shapeTool, colorTool, color);
}

static bool IsFreeShape (const TDF_Label& label, const Handle (XCAFDoc_ShapeTool)& shapeTool)
{
    TopoDS_Shape tmpShape;
    return shapeTool->GetShape (label, tmpShape) && shapeTool->IsFree (label);
}

class XcafFace : public OcctFace
{
public:
    XcafFace (const TopoDS_Face& face, const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool) :
        OcctFace (face),
        shapeTool (shapeTool),
        colorTool (colorTool)
    {

    }

    virtual bool GetColor (Color& color) const override
    {
        return GetShapeColor ((const TopoDS_Shape&) face, shapeTool, colorTool, color);
    }

private:
    const Handle (XCAFDoc_ShapeTool)& shapeTool;
    const Handle (XCAFDoc_ColorTool)& colorTool;
};

class XcafShapeMesh : public Mesh
{
public:
    XcafShapeMesh (const TopoDS_Shape& shape, const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool) :
        Mesh (),
        shape (shape),
        shapeTool (shapeTool),
        colorTool (colorTool)
    {

    }

    virtual std::string GetName () const override
    {
        return GetShapeName (shape, shapeTool);
    }

    virtual bool GetColor (Color& color) const override
    {
        return GetShapeColor (shape, shapeTool, colorTool, color);
    }

    virtual void EnumerateFaces (const std::function<void (const Face& face)>& onFace) const override
    {
        for (TopExp_Explorer ex (shape, TopAbs_FACE); ex.More (); ex.Next ()) {
            const TopoDS_Face& face = TopoDS::Face (ex.Current ());
            XcafFace outputFace (face, shapeTool, colorTool);
            onFace (outputFace);
        }
    }

private:
    const TopoDS_Shape& shape;
    const Handle (XCAFDoc_ShapeTool)& shapeTool;
    const Handle (XCAFDoc_ColorTool)& colorTool;
};

class XcafStandaloneFacesMesh : public Mesh
{
public:
    XcafStandaloneFacesMesh (const TopoDS_Shape& shape, const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool) :
        Mesh (),
        shape (shape),
        shapeTool (shapeTool),
        colorTool (colorTool)
    {

    }

    bool HasFaces () const
    {
        TopExp_Explorer ex (shape, TopAbs_FACE, TopAbs_SHELL);
        return ex.More ();
    }

    virtual std::string GetName () const override
    {
        return std::string ();
    }

    virtual bool GetColor (Color&) const override
    {
        return false;
    }

    virtual void EnumerateFaces (const std::function<void (const Face& face)>& onFace) const override
    {
        for (TopExp_Explorer ex (shape, TopAbs_FACE, TopAbs_SHELL); ex.More (); ex.Next ()) {
            const TopoDS_Face& face = TopoDS::Face (ex.Current ());
            XcafFace outputFace (face, shapeTool, colorTool);
            onFace (outputFace);
        }
    }

private:
    const TopoDS_Shape& shape;
    const Handle (XCAFDoc_ShapeTool)& shapeTool;
    const Handle (XCAFDoc_ColorTool)& colorTool;
};

class XcafNode : public Node
{
public:
    XcafNode (const TDF_Label& label, const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool, int depth = 0, int maxDepth = 0) :
        label (label),
        shapeTool (shapeTool),
        colorTool (colorTool),
        currentDepth (depth),
        maxHierarchyDepth (maxDepth)
    {

    }

    virtual std::string GetName () const override
    {
        return GetLabelName (label, shapeTool);
    }

    virtual std::vector<NodePtr> GetChildren () const override
    {
        if (IsMeshNode ()) {
            return {};
        }

        std::vector<NodePtr> children;
        for (TDF_ChildIterator it (label); it.More (); it.Next ()) {
            TDF_Label childLabel = it.Value ();
            if (IsFreeShape (childLabel, shapeTool)) {
                children.push_back (std::make_shared<const XcafNode> (
                    childLabel, shapeTool, colorTool, currentDepth + 1, maxHierarchyDepth
                    ));
            }
        }
        return children;
    }

    virtual bool IsMeshNode () const override
    {
        // Hierarchy depth limit: if we've reached max depth, treat as mesh to merge deep sub-parts
        if (maxHierarchyDepth > 0 && currentDepth >= maxHierarchyDepth) {
            return true;
        }

        // if there are no children, it is a mesh node
        if (!label.HasChild ()) {
            return true;
        }

        // if it has a subshape child, treat it as mesh node
        bool hasSubShapeChild = false;
        for (TDF_ChildIterator it (label); it.More (); it.Next ()) {
            TDF_Label childLabel = it.Value ();
            if (shapeTool->IsSubShape (childLabel)) {
                hasSubShapeChild = true;
                break;
            }
        }
        if (hasSubShapeChild) {
            return true;
        }

        // if it doesn't have a freeshape child, treat it as a mesh node
        bool hasFreeShapeChild = false;
        for (TDF_ChildIterator it (label); it.More (); it.Next ()) {
            TDF_Label childLabel = it.Value ();
            if (IsFreeShape (childLabel, shapeTool)) {
                hasFreeShapeChild = true;
                break;
            }
        }
        if (!hasFreeShapeChild) {
            return true;
        }

        return false;
    }

    virtual void EnumerateMeshes (const std::function<void (const Mesh&)>& onMesh) const override
    {
        if (!IsMeshNode ()) {
            return;
        }

        // Use const reference to avoid unnecessary copy (shape is reference-counted but still has overhead)
        const TopoDS_Shape& shape = shapeTool->GetShape (label);
        EnumerateShapeMeshes (shape, onMesh);
    }

private:
    void EnumerateShapeMeshes (const TopoDS_Shape& shape, const std::function<void (const Mesh&)>& onMesh) const
    {
        // Enumerate solids
        for (TopExp_Explorer ex (shape, TopAbs_SOLID); ex.More (); ex.Next ()) {
            const TopoDS_Shape& currentShape = ex.Current ();
            XcafShapeMesh outputShapeMesh (currentShape, shapeTool, colorTool);
            onMesh (outputShapeMesh);
        }

        // Enumerate shells that are not part of a solid
        for (TopExp_Explorer ex (shape, TopAbs_SHELL, TopAbs_SOLID); ex.More (); ex.Next ()) {
            const TopoDS_Shape& currentShape = ex.Current ();
            XcafShapeMesh outputShapeMesh (currentShape, shapeTool, colorTool);
            onMesh (outputShapeMesh);
        }

        // Create a mesh from faces that are not part of a shell
        XcafStandaloneFacesMesh standaloneFacesMesh (shape, shapeTool, colorTool);
        if (standaloneFacesMesh.HasFaces ()) {
            onMesh (standaloneFacesMesh);
        }
    }

    TDF_Label label;
    const Handle (XCAFDoc_ShapeTool)& shapeTool;
    const Handle (XCAFDoc_ColorTool)& colorTool;
    int currentDepth;
    int maxHierarchyDepth;
};

class XcafRootNode : public Node
{
public:
    XcafRootNode (const Handle (XCAFDoc_ShapeTool)& shapeTool, const Handle (XCAFDoc_ColorTool)& colorTool, const ImportParams& params) :
        shapeTool (shapeTool),
        colorTool (colorTool),
        params (params)
    {

    }

    virtual std::string GetName () const override
    {
        return std::string ();
    }

    virtual std::vector<NodePtr> GetChildren () const override
    {
        TDF_Label mainLabel = shapeTool->Label ();

        // Collect all free shape labels first
        std::vector<TDF_Label> freeShapeLabels;
        for (TDF_ChildIterator it (mainLabel); it.More (); it.Next ()) {
            TDF_Label childLabel = it.Value ();
            if (IsFreeShape (childLabel, shapeTool)) {
                freeShapeLabels.push_back (childLabel);
            }
        }

        // Parallel triangulation for better performance on multi-core systems
        const size_t numShapes = freeShapeLabels.size ();
        const unsigned int hardwareConcurrency = std::thread::hardware_concurrency ();
        const unsigned int numThreads = (hardwareConcurrency > 0) ? hardwareConcurrency : 4;

        // Only use parallel processing if we have enough shapes (avoid threading overhead for small models)
        const bool useParallel = (numShapes >= 10 && numThreads > 1);

        if (useParallel) {
            // Parallel triangulation
            std::vector<std::thread> workers;
            std::mutex shapeMutex;
            size_t nextShapeIndex = 0;

            for (unsigned int i = 0; i < numThreads && i < numShapes; ++i) {
                workers.emplace_back ([&]() {
                    while (true) {
                        size_t index;
                        {
                            std::lock_guard<std::mutex> lock (shapeMutex);
                            if (nextShapeIndex >= numShapes) {
                                break;
                            }
                            index = nextShapeIndex++;
                        }

                        TDF_Label childLabel = freeShapeLabels[index];
                        TopoDS_Shape shape = shapeTool->GetShape (childLabel);
                        TriangulateShape (shape, params);
                    }
                });
            }

            for (auto& worker : workers) {
                worker.join ();
            }
        } else {
            // Sequential triangulation for small models
            for (const TDF_Label& childLabel : freeShapeLabels) {
                TopoDS_Shape shape = shapeTool->GetShape (childLabel);
                TriangulateShape (shape, params);
            }
        }

        // Build node hierarchy (must be sequential as we're building shared_ptrs)
        std::vector<NodePtr> children;
        for (const TDF_Label& childLabel : freeShapeLabels) {
            children.push_back (std::make_shared<const XcafNode> (
                childLabel, shapeTool, colorTool, 1, params.maxHierarchyDepth
            ));
        }

        return children;
    }

    virtual bool IsMeshNode () const override
    {
        return false;
    }

    virtual void EnumerateMeshes (const std::function<void (const Mesh&)>&) const override
    {

    }

private:
    const Handle (XCAFDoc_ShapeTool)& shapeTool;
    const Handle (XCAFDoc_ColorTool)& colorTool;
    const ImportParams& params;
};

ImporterXcaf::ImporterXcaf () :
    Importer (),
    document (nullptr),
    shapeTool (nullptr),
    colorTool (nullptr),
    rootNode (nullptr)
{

}

Importer::Result ImporterXcaf::LoadFile (const std::vector<std::uint8_t>& fileContent, const ImportParams& params)
{
    document = new TDocStd_Document ("XmlXCAF");

    UnitsMethods_LengthUnit lengthUnit = LinearUnitToLengthUnit (params.linearUnit);
    XCAFDoc_DocumentTool::SetLengthUnit (document, 1.0, lengthUnit);

    if (!TransferToDocument (fileContent)) {
        return Importer::Result::ImportFailed;
    }

    TDF_Label mainLabel = document->Main ();
    shapeTool = XCAFDoc_DocumentTool::ShapeTool (mainLabel);
    colorTool = XCAFDoc_DocumentTool::ColorTool (mainLabel);

    TDF_LabelSequence labels;
    shapeTool->GetFreeShapes (labels);
    if (labels.IsEmpty ()) {
        return Importer::Result::ImportFailed;
    }

    rootNode = std::make_shared<const XcafRootNode> (shapeTool, colorTool, params);
    return Importer::Result::Success;
}

NodePtr ImporterXcaf::GetRootNode () const
{
    return rootNode;
}
