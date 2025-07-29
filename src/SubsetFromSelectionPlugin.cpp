#include "SubsetFromSelectionPlugin.h"
#include "SubsetFromSelectionTransformationDialogs.h"

#include <PointData/PointData.h>

#include <QDebug>
#include <QtCore>

#include <cmath>

Q_PLUGIN_METADATA(IID "studio.manivault.SubsetFromSelectionPlugin")

using namespace mv;
using namespace mv::util;

const QMap<SubsetFromSelectionPlugin::Type, QString> SubsetFromSelectionPlugin::types = QMap<SubsetFromSelectionPlugin::Type, QString>({
    { SubsetFromSelectionPlugin::Type::Simple, "Create subset" },
    { SubsetFromSelectionPlugin::Type::Extended, "Create subset with extended options" }
    });


SubsetFromSelectionPlugin::SubsetFromSelectionPlugin(const PluginFactory* factory) :
    TransformationPlugin(factory),
    _type(Type::Simple)
{

}



void SubsetFromSelectionPlugin::transform()
{

    if (getInputDatasets().isEmpty())
        return;

    switch (_type)
    {
        // Transform points in place
        case SubsetFromSelectionPlugin::Type::Simple:
        {
            
            for (auto& points : getInputDatasets()) {
                if (!points.isValid()) {
                    continue;
                }
                auto selectedIndices = points->getSelectionIndices();
                if (selectedIndices.empty()) {
                    qWarning() << "No points selected for dataset: "+ points->getGuiName();
                    continue;
                }
                auto& datasetTask = points->getTask();
                datasetTask.setName("Transforming");
                datasetTask.setRunning();
                datasetTask.setProgressDescription(QString("%1 transformation").arg(getTypeName(_type)));

                datasetTask.setProgress(0.0f);
                points->setLocked(true);

                points->createSubsetFromSelection(points->getGuiName()+"_Subset", points);
                points.setProperty("Last transformed by", getName());
                points->setLocked(false);
                datasetTask.setProgress(1.0f);
                datasetTask.setFinished();
            }
            break; 
        }
        // Create new data set
        case SubsetFromSelectionPlugin::Type::Extended:
        {
            TransformationDialog dialog;
            if (dialog.exec() != QDialog::Accepted) {
                return;
            }
            SubsetOptions options = dialog.getOptions();

            // Use options in your logic:
            for (Dataset<Points> points : getInputDatasets()) {
                if (!points.isValid()) {
                    continue;
                }
                auto selectedIndices = points->getSelectionIndices();
                if (selectedIndices.empty()) {
                    qWarning() << "No points selected for dataset: " + points->getGuiName();
                    continue;
                }
                QString subsetName =  points->getGuiName() + "_Subset";
                auto& datasetTask = points->getTask();
                datasetTask.setName("Transforming");
                datasetTask.setRunning();
                datasetTask.setProgressDescription(QString("%1 transformation").arg(getTypeName(_type)));
                datasetTask.setProgress(0.0f);
                points->setLocked(true);
                if (options.includeChildren) 
                {
                    createSubsetWithChildren(points);
                } 
                else 
                {
                    if (options.createSubsetAsChild) 
                    {
                        points->createSubsetFromSelection(points->getGuiName() + "_Subset", points);

                    } 
                    else 
                    {
                        points->createSubsetFromSelection(points->getGuiName() + "_Subset");

                    }
                }
                
                points.setProperty("Last transformed by", getName());
                points->setLocked(false);
                datasetTask.setProgress(1.0f);
                datasetTask.setFinished();
            }
            break;
        }
        default:
            break;
    }

}


void SubsetFromSelectionPlugin::createSubsetWithChildren(Dataset<Points>& inputPointsDataset)
{

    mv::Datasets datasetsToNotify;

    if (!inputPointsDataset.isValid()) {
        qWarning() << "Invalid input dataset for SubsetFromSelectionPlugin";
        return;
    }
    auto childDatasets = inputPointsDataset->getChildren();

    int numDimensions = inputPointsDataset->getNumDimensions();
    auto dimensionNames = inputPointsDataset->getDimensionNames();
    auto selectionIndices = inputPointsDataset->getSelectionIndices();
    if (selectionIndices.empty()) {
        qWarning() << "No points selected for dataset: " + inputPointsDataset->getGuiName();
        return;
    }

    std::vector<int> allDimensionIndices(numDimensions);
    std::iota(allDimensionIndices.begin(), allDimensionIndices.end(), 0);

    QString newDatasetName = inputPointsDataset->getGuiName() + "_Subset";

    Dataset<Points> clusterPointsDataset = mv::data().createDataset("Points", newDatasetName);
    events().notifyDatasetAdded(clusterPointsDataset);

    // Extract and set the data for the selected cluster indices
    std::vector<float> clusterPointsData(selectionIndices.size() * numDimensions);
    inputPointsDataset->populateDataForDimensions(clusterPointsData, allDimensionIndices, selectionIndices);
    clusterPointsDataset->setData(clusterPointsData.data(), selectionIndices.size(), numDimensions);
    clusterPointsDataset->setDimensionNames(dimensionNames);
    datasetsToNotify.push_back(clusterPointsDataset);
    std::unordered_map<int, int> splitIndicesMap;

    for (int i = 0; i < selectionIndices.size(); i++) {
        splitIndicesMap.insert({ selectionIndices[i], i });
    }
    for (const Dataset<Clusters>& child : childDatasets) {

        if (child->getDataType() == PointType) {
            Dataset<Points> fullChildPoints = child->getFullDataset<Points>();
            if (!fullChildPoints.isValid()) {
                continue;
            }
            fullChildPoints->setLocked(true);
            Dataset<Points> childClusterPoints = mv::data().createDerivedDataset(
                child->getGuiName(),
                clusterPointsDataset
            );
            events().notifyDatasetAdded(childClusterPoints);

            int childNumDimensions = fullChildPoints->getNumDimensions();
            std::vector<int> childDimensionIndices(childNumDimensions);
            std::iota(childDimensionIndices.begin(), childDimensionIndices.end(), 0);

            std::vector<float> childClusterData(selectionIndices.size() * childNumDimensions);
            fullChildPoints->populateDataForDimensions(childClusterData, childDimensionIndices, selectionIndices);
            childClusterPoints->setData(childClusterData.data(), selectionIndices.size(), childNumDimensions);
            childClusterPoints->setDimensionNames(fullChildPoints->getDimensionNames());
            datasetsToNotify.push_back(childClusterPoints);
            fullChildPoints->setLocked(false);
        }

        else if (child->getDataType() == ClusterType) {
            Dataset<Clusters> fullChildClusters = child->getFullDataset<Clusters>();
            if (!fullChildClusters.isValid()) {
                continue;
            }
            fullChildClusters->setLocked(true);
            Dataset<Clusters> childClusterDataset = mv::data().createDataset(
                "Cluster",
                child->getGuiName(),
                clusterPointsDataset
            );
            events().notifyDatasetAdded(childClusterDataset);

            // For each cluster in the child, remap indices to the new cluster subset
            for (const auto& cluster : fullChildClusters->getClusters()) {
                std::vector<std::seed_seq::result_type> remappedIndices;
                const auto& originalIndices = cluster.getIndices();
                for (int idx : originalIndices) {
                    // Only include indices that are present in the selected cluster
                    if (auto it = splitIndicesMap.find(idx); it != splitIndicesMap.end()) {
                        remappedIndices.push_back(it->second);
                    }
                }
                Cluster remappedCluster = cluster;
                remappedCluster.setIndices(remappedIndices);
                if (remappedCluster.getIndices().empty()) {
                    continue; // Skip empty clusters
                }
                childClusterDataset->addCluster(remappedCluster);



            }

            datasetsToNotify.push_back(childClusterDataset);
            fullChildClusters->setLocked(false);
        }

    }
    for (const auto& dataset : datasetsToNotify) {
        events().notifyDatasetDataChanged(dataset);
    }
 
}

SubsetFromSelectionPlugin::Type SubsetFromSelectionPlugin::getType() const
{
    return _type;
}

void SubsetFromSelectionPlugin::setType(const Type& type)
{
    if (type == _type)
        return;

    _type = type;
}

QString SubsetFromSelectionPlugin::getTypeName(const Type& type)
{
    return types[type];
}

// =============================================================================
// Plugin Factory 
// =============================================================================

SubsetFromSelectionPluginFactory::SubsetFromSelectionPluginFactory()
{
    getPluginMetadata().setDescription("SubsetFromSelectionPlugin");
    getPluginMetadata().setSummary("This plugin creates a subset from a point dataset.");
    getPluginMetadata().setCopyrightHolder({ "BioVault (Biomedical Visual Analytics Unit LUMC - TU Delft)" });
    getPluginMetadata().setAuthors({
    });
    getPluginMetadata().setOrganizations({
        { "LUMC", "Leiden University Medical Center", "https://www.lumc.nl/en/" },
        { "TU Delft", "Delft university of technology", "https://www.tudelft.nl/" }
	});
    getPluginMetadata().setLicenseText("This plugin is distributed under the [LGPL v3.0](https://www.gnu.org/licenses/lgpl-3.0.en.html) license.");
}

SubsetFromSelectionPlugin* SubsetFromSelectionPluginFactory::produce()
{
    // Return a new instance of the transformation plugin
    return new SubsetFromSelectionPlugin(this);
}

mv::DataTypes SubsetFromSelectionPluginFactory::supportedDataTypes() const
{
    DataTypes supportedTypes;

    // This transformation plugin is compatible with points datasets
    supportedTypes.append(PointType);

    return supportedTypes;
}

mv::gui::PluginTriggerActions SubsetFromSelectionPluginFactory::getPluginTriggerActions(const mv::Datasets& datasets) const
{
    mv::gui::PluginTriggerActions pluginTriggerActions;

    const auto numberOfDatasets = datasets.count();

    if (PluginFactory::areAllDatasetsOfTheSameType(datasets, PointType)) {
        if (numberOfDatasets >= 1 && datasets.first()->getDataType() == PointType) {
            const auto addPluginTriggerAction = [this, &pluginTriggerActions, datasets](const SubsetFromSelectionPlugin::Type& type) -> void {
                const auto typeName = SubsetFromSelectionPlugin::getTypeName(type);

                auto pluginTriggerAction = new mv::gui::PluginTriggerAction(const_cast<SubsetFromSelectionPluginFactory*>(this), this, QString("SubsetFromSelection/%1").arg(typeName), QString("Perform %1 data transformation").arg(typeName), icon(), [this, datasets, type](mv::gui::PluginTriggerAction& pluginTriggerAction) -> void {
                    auto pluginInstance = dynamic_cast<SubsetFromSelectionPlugin*>(plugins().requestPlugin(getKind()));

                    pluginInstance->setInputDatasets(datasets);
                    pluginInstance->setType(type);
                    pluginInstance->transform();
                    });

                pluginTriggerActions << pluginTriggerAction;
            };

            addPluginTriggerAction(SubsetFromSelectionPlugin::Type::Simple);
            addPluginTriggerAction(SubsetFromSelectionPlugin::Type::Extended);
        }
    }

    return pluginTriggerActions;
}

