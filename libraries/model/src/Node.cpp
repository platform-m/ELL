////////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Project:  Embedded Learning Library (ELL)
//  File:     Node.cpp (model)
//  Authors:  Chuck Jacobs
//
////////////////////////////////////////////////////////////////////////////////////////////////////

#include "Node.h"
#include "InputPort.h"
#include "Model.h"
#include "ModelTransformer.h"
#include "OutputPort.h"

#include <utilities/include/IArchivable.h>

#include <unordered_set>

namespace ell
{
namespace model
{
    namespace
    {
        //
        // Relevant archive format versions
        //
        constexpr utilities::ArchiveVersion noMetadataArchiveVersion = { utilities::ArchiveVersionNumbers::v0_initial };
        constexpr utilities::ArchiveVersion metadataArchiveVersion = { utilities::ArchiveVersionNumbers::v3_model_metadata };
    } // namespace

    Node::Node(const std::vector<InputPortBase*>& inputs, const std::vector<OutputPortBase*>& outputs) :
        _id(NodeId()),
        _inputs(inputs),
        _outputs(outputs)
    {
    }

    void Node::AddInputPort(InputPortBase* input)
    {
        _inputs.push_back(input);
    }

    void Node::AddOutputPort(OutputPortBase* output)
    {
        _outputs.push_back(output);
    }

    InputPortBase* Node::GetInputPort(const std::string& portName)
    {
        for (auto port : _inputs)
        {
            if (port->GetName() == portName)
            {
                return port;
            }
        }
        return nullptr;
    }

    const InputPortBase* Node::GetInputPort(const std::string& portName) const
    {
        for (auto port : _inputs)
        {
            if (port->GetName() == portName)
            {
                return port;
            }
        }
        return nullptr;
    }

    InputPortBase* Node::GetInputPort(size_t portIndex)
    {
        return _inputs[portIndex];
    }

    const InputPortBase* Node::GetInputPort(size_t portIndex) const
    {
        return _inputs[portIndex];
    }

    bool Node::CanAcceptInputLayout(const utilities::DimensionOrder& order) const
    {
        return _inputs.empty() ||
               std::all_of(_inputs.begin(), _inputs.end(), [&order](const auto port) {
                   return port->GetMemoryLayout().GetLogicalDimensionOrder() == order;
               });
    }

    OutputPortBase* Node::GetOutputPort(const std::string& portName)
    {
        for (auto port : _outputs)
        {
            if (port->GetName() == portName)
            {
                return port;
            }
        }
        return nullptr;
    }

    const OutputPortBase* Node::GetOutputPort(const std::string& portName) const
    {
        for (auto port : _outputs)
        {
            if (port->GetName() == portName)
            {
                return port;
            }
        }
        return nullptr;
    }

    OutputPortBase* Node::GetOutputPort(size_t portIndex)
    {
        return _outputs[portIndex];
    }

    const OutputPortBase* Node::GetOutputPort(size_t portIndex) const
    {
        return _outputs[portIndex];
    }

    bool Node::TrySetOutputLayout(const utilities::DimensionOrder& order)
    {
        for (auto port : _outputs)
        {
            auto oldLayout = port->GetMemoryLayout();
            port->SetMemoryLayout(oldLayout.ReorderedCopy(order));
        }

        return true;
    }

    Port* Node::GetPort(const std::string& portName)
    {
        auto inputPort = GetInputPort(portName);
        if (inputPort != nullptr)
        {
            return inputPort;
        }

        return GetOutputPort(portName);
    }

    const Port* Node::GetPort(const std::string& portName) const
    {
        auto inputPort = GetInputPort(portName);
        if (inputPort != nullptr)
        {
            return inputPort;
        }

        return GetOutputPort(portName);
    }

    std::vector<const Node*> Node::GetParentNodes() const
    {
        std::unordered_set<const Node*> nodes;
        for (const auto& port : _inputs)
        {
            for (const auto& node : port->GetParentNodes())
            {
                nodes.insert(node);
            }
        }
        return std::vector<const Node*>{ nodes.begin(), nodes.end() };
    }

    std::vector<const Node*> Node::GetDependentNodes() const
    {
        std::unordered_set<const Node*> nodes;
        for (const auto& port : _outputs)
        {
            for (const auto& referencingPort : port->GetReferences())
            {
                nodes.insert(referencingPort->GetNode());
            }
        }
        return std::vector<const Node*>{ nodes.begin(), nodes.end() };
    }

    // Default implementation of Refine just copies and returns false
    bool Node::Refine(ModelTransformer& transformer) const
    {
        Copy(transformer);
        return false;
    }

    void Node::Print(std::ostream& os) const
    {
        bool isFirstInputPort = true;
        os << "node_" << GetId() << " (" << std::hex << this << std::dec << ") = " << GetRuntimeTypeName() << "(";
        for (const auto& inputPort : GetInputPorts())
        {
            os << (isFirstInputPort ? "" : ", ");
            isFirstInputPort = false;

            auto elements = model::PortElementsBase{ inputPort->GetReferencedPort() };
            if (elements.NumRanges() > 1)
            {
                os << "{";
            }

            bool isFirstRange = true;
            for (const auto& range : elements.GetRanges())
            {
                os << (isFirstRange ? "" : ", ");
                isFirstRange = false;

                auto port = range.ReferencedPort();
                os << "node_" << port->GetNode()->GetId() << "." << port->GetName();
                if (!range.IsFullPortRange())
                {
                    auto start = range.GetStartIndex();
                    auto size = range.Size();
                    os << "[" << start << ":" << (start + size) << "]";
                }
            }

            if (elements.NumRanges() > 1)
            {
                os << "}";
            }
        }
        os << ")" << std::endl;
    }

    void Node::UpdateInputPorts()
    {
        for (auto input : _inputs)
        {
            input->UpdateReferencedPort();
        }
    }

    utilities::ArchiveVersion Node::GetArchiveVersion() const
    {
        if (_metadata.IsEmpty())
        {
            return noMetadataArchiveVersion;
        }
        else
        {
            return metadataArchiveVersion;
        }
    }

    bool Node::CanReadArchiveVersion(const utilities::ArchiveVersion& version) const
    {
        return version >= noMetadataArchiveVersion && version <= metadataArchiveVersion;
    }

    void Node::WriteToArchive(utilities::Archiver& archiver) const
    {
        archiver["id"] << _id;
        if (!_metadata.IsEmpty())
        {
            archiver["metadata"] << _metadata;
        }
    }

    void Node::ReadFromArchive(utilities::Unarchiver& archiver)
    {
        NodeId oldId;
        archiver["id"] >> oldId;
        _id = oldId;
        archiver.OptionalProperty("metadata") >> _metadata;

        auto& context = archiver.GetContext();
        ModelSerializationContext& newContext = dynamic_cast<ModelSerializationContext&>(context);
        newContext.MapNode(oldId, this);
    }

    void Node::SetId(Node::NodeId id)
    {
        _id = id;
    }

    void Node::SetModel(Model* model)
    {
        if (_model)
        {
            throw utilities::LogicException(utilities::LogicExceptionErrors::illegalState, "Setting model on a node already assigned to a model");
        }
        _model = std::make_unique<Model>(model->ShallowCopy());
    }

    std::string Node::GetFriendlyName() const
    {
        auto& meta = this->GetMetadata();
        if (meta.HasEntry("name"))
        {
            return meta.GetEntry<std::string>("name");
        }
        return "";
    }

} // namespace model
} // namespace ell
