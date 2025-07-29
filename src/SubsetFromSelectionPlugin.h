#pragma once

#include <TransformationPlugin.h>

#include <QMap>
#include <QString>
#include <PointData/PointData.h> 
#include <ClusterData/ClusterData.h> 
/** All plugin related classes are in the ManiVault plugin namespace */
using namespace mv::plugin;

/**
 * transformation plugin class
 *
 * This transformation plugin class provides skeleton code that shows how to develop 
 * an transformation plugin in ManiVault.
 * 
 * In contrast to analysis plugins, transformation do not create an output data set by default,
 * but operate on the input data set. In this plugin, we provide to transformation options,
 * either simply taking the absolute value of each data point or raising it to the power of 2.
 *
 * To see the plugin in action, please follow the steps below:
 *
 * 1. This plugin works on points datasets and is created by right-clicking a points
 * dataset in the data hierarchy viewer and choosing Transform >>  transformation.
 * 2. Chose the transformation option
 * 
 */
class SubsetFromSelectionPlugin : public TransformationPlugin
{
Q_OBJECT

public:

    /** Define the transformation options */
    enum class Type {
        Simple,       /** absolute value */
        Extended       /** value squared */
    };

    static const QMap<Type, QString> types;

public:

    /**
     * Constructor
     * @param factory Pointer to the plugin factory
     */
    SubsetFromSelectionPlugin(const PluginFactory* factory);

    /** Destructor */
    ~SubsetFromSelectionPlugin() override = default;

    /** Initialization is called when the plugin is first instantiated. */
    void init() override {};

    /** Performs the data transformation */
    void transform() override;

    /**
     * Transform points dataset
     */
    void createSubsetWithChildren(Dataset<Points>& points);

    /**
     * Get transformation type
     * @return transformation type
     */
    Type getType() const;

    /**
     * Set transformation type
     * @param type transformation type
     */
    void setType(const Type& type);

    /**
     * Get string representation of transformation type enum
     * @param type transformation type
     * @return Type name
     */
    static QString getTypeName(const Type& type);

private:
    Type    _type;      /** Data conversion type */

};

/**
 *  transform plugin factory class
 *
 * Note: Factory does not need to be altered (merely responsible for generating new plugins when requested)
 */
class SubsetFromSelectionPluginFactory : public TransformationPluginFactory
{
    Q_INTERFACES(mv::plugin::TransformationPluginFactory mv::plugin::PluginFactory)
    Q_OBJECT
    Q_PLUGIN_METADATA(IID   "studio.manivault.SubsetFromSelectionPlugin"
                      FILE  "PluginInfo.json")

public:

    /** Default constructor */
    SubsetFromSelectionPluginFactory();

    /** Creates an instance of the  transform plugin */
    SubsetFromSelectionPlugin* produce() override;

    /** Returns the data types that are supported by the  transformation plugin */
    mv::DataTypes supportedDataTypes() const override;

    /** Enable right-click on data set to execute transformation */
    mv::gui::PluginTriggerActions getPluginTriggerActions(const mv::Datasets& datasets) const override;

};
