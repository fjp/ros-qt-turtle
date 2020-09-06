#include "rqt_turtle/turtle_plugin.h"
#include <pluginlib/class_list_macros.h>
#include <QStringList>
#include <QInputDialog>  // TODO remove
#include <QColorDialog>
#include <QVariantMap>
#include <QTreeWidgetItem>

#include <std_srvs/Empty.h>
#include <turtlesim/Spawn.h>
#include <turtlesim/Kill.h>
#include <turtlesim/TeleportAbsolute.h>
#include <ros/service.h>
#include <ros/param.h>


#include "ui_turtle_plugin.h"


#include "rqt_turtle/service_caller.h"

namespace rqt_turtle {

    TurtlePlugin::TurtlePlugin()
        : rqt_gui_cpp::Plugin()
        , m_pUi(new Ui::TurtlePluginWidget)
        , m_pWidget(0)
    {
        // Constructor is called first before initPlugin function, needless to say.

        // give QObjects reasonable names
        setObjectName("TurtlePlugin");

        //m_pServiceCaller = new ServiceCaller(m_pWidget);
    }

    void TurtlePlugin::initPlugin(qt_gui_cpp::PluginContext& context)
    {
        // access standalone command line arguments
        QStringList argv = context.argv();
        // create QWidget
        m_pWidget = new QWidget();
        // extend the widget with all attributes and children from UI file
        //m_pUi.setupUi(widget_);
        ROS_INFO("INIT");
        m_pUi->setupUi(m_pWidget);
        // add widget to the user interface
        context.addWidget(m_pWidget);

        connect(m_pUi->btnReset, SIGNAL(clicked()), this, SLOT(on_btnReset_clicked()));
        connect(m_pUi->btnSpawn, SIGNAL(clicked()), this, SLOT(on_btnSpawn_clicked()));
        connect(m_pUi->btnKill, SIGNAL(clicked()), this, SLOT(on_btnKill_clicked()));
        connect(m_pUi->btnColor, SIGNAL(clicked()), this, SLOT(on_btnColor_clicked()));
        connect(m_pUi->btnDraw, SIGNAL(clicked()), this, SLOT(on_btnDraw_clicked()));
        connect(m_pUi->btnTeleportAbs, SIGNAL(clicked()), this, SLOT(on_btnTeleportAbs_clicked()));

        connect(m_pUi->treeTurtles, SIGNAL(itemSelectionChanged()), 
                this, SLOT(on_selection_changed()));

        
        //m_pServiceCallerDialog = new QDialog(0,0);
        //m_pUiServiceCallerWidget->setupUi(m_pServiceCallerDialog);

        //m_pServiceCallerDialog->show();
    }

    void TurtlePlugin::shutdownPlugin()
    {
        // TODO unregister all publishers here
    }

    void TurtlePlugin::saveSettings(qt_gui_cpp::Settings& plugin_settings, qt_gui_cpp::Settings& instance_settings) const
    {
        // TODO save intrinsic configuration, usually using:
        // instance_settings.setValue(k, v)
    }

    void TurtlePlugin::restoreSettings(const qt_gui_cpp::Settings& plugin_settings, const qt_gui_cpp::Settings& instance_settings)
    {
        // TODO restore intrinsic configuration, usually using:
        // v = instance_settings.value(k)
    }

    /*bool hasConfiguration() const
    {
        return true;
    }

    void triggerConfiguration()
    {
        // Usually used to open a dialog to offer the user a set of configuration
    }*/

    void TurtlePlugin::on_btnReset_clicked()
    {
        ROS_INFO("Reset turtlesim.");
        std_srvs::Empty empty;
        ros::service::call<std_srvs::Empty>("reset", empty);

        // Clear the listViewWidget
        m_pUi->treeTurtles->clear();
    }

    void TurtlePlugin::on_btnSpawn_clicked()
    {
        ROS_DEBUG("Spawn clicked");
        std::string service_name = "/spawn";
        m_pServiceCaller = new ServiceCaller(m_pWidget, service_name);

        QString qstrTurtleName;
        QVariantMap request;
        bool ok = m_pServiceCaller->exec() == QDialog::Accepted;
        if (ok)
        {
            ROS_INFO("accepted");
            request = m_pServiceCaller->getRequest();
            qstrTurtleName = request["name"].toString();
        }

        if (!ok || qstrTurtleName.isEmpty())
        {
            ROS_INFO("Closed Service Caller Dialog or Turtle Name empty.");
            return;
        }
        auto existingTurtles = m_pUi->treeTurtles->findItems(qstrTurtleName, Qt::MatchExactly);
        const char * strTurtleName = qstrTurtleName.toStdString().c_str();
        if (existingTurtles.size() > 0)
        {
            ROS_INFO("Turtle with the name \"%s\" already exists.", strTurtleName);
            return;
        }

        ROS_INFO("Spawn turtle: %s.", strTurtleName);
        turtlesim::Spawn spawn;
        spawn.request.x = request["x"].toString().toFloat();
        spawn.request.y = request["y"].toString().toFloat();
        spawn.request.theta = request["theta"].toFloat();
        spawn.request.name = request["name"].toString().toStdString().c_str();
        ros::service::call<turtlesim::Spawn>("spawn", spawn);
        //m_pUi->tableTurtles->addItems(QStringList(qstrTurtleName));

        QTreeWidgetItem *item = new QTreeWidgetItem(m_pUi->treeTurtles);
        item->setText(0, qstrTurtleName); // Column 0 name
        item->setText(1, request["x"].toString()); // Column 1 x
        item->setText(2, request["y"].toString()); // Column 2 y
        item->setText(3, request["theta"].toString()); // Column 3 theta
        // TODO pen on/off
        m_pUi->treeTurtles->insertTopLevelItem(0, item);
    }

    void TurtlePlugin::on_btnColor_clicked()
    {
        int r, g, b;
        ros::param::get("/turtlesim/background_b", b);
        ros::param::get("/turtlesim/background_g", g);
        ros::param::get("/turtlesim/background_r", r);
        ROS_INFO("Current color (r,g,b) = (%i,%i,%i)", r, g, b);
        QColor qColor = QColorDialog::getColor();
        ROS_INFO("Color %s", qColor.name().toStdString().c_str());
        qColor.getRgb(&r, &g, &b);
        ROS_INFO("Setting color to (r,g,b) = (%i,%i,%i)", r, g, b);
        ros::param::set("/turtlesim/background_b", b);
        ros::param::set("/turtlesim/background_g", g);
        ros::param::set("/turtlesim/background_r", r);

        // Note: this will not set the color (only after reset is called)
    }

    void TurtlePlugin::on_btnDraw_clicked()
    {
        auto list = m_pUi->treeTurtles->selectedItems();
        ROS_INFO("%d", list.size());
        if (list.size() > 0)
        {
            QString turtleName = list[0]->text(0);
            ROS_INFO(turtleName.toStdString().c_str());

        }
    }

    void TurtlePlugin::on_btnKill_clicked()
    {
        ROS_INFO("Killing turtle %s", m_strSelectedTurtle.c_str());
        turtlesim::Kill kill;
        kill.request.name = m_strSelectedTurtle;
        ros::service::call<turtlesim::Kill>("kill", kill);

        // remove turtle from tree widget
        auto list = m_pUi->treeTurtles->findItems(QString::fromStdString(m_strSelectedTurtle), Qt::MatchExactly);
        for (auto item : list)
        {
            delete item;
        }
        //m_pUi->treeTurtles->addItems(QStringList(qstrTurtleName));
    }

    void TurtlePlugin::on_btnTeleportAbs_clicked()
    {
        std::string strServiceName = "/" + m_strSelectedTurtle + "/teleport_absolute";
        turtlesim::TeleportAbsolute sTeleportAbsolute;
        auto request = sTeleportAbsolute.request;
        request.x = 1.0;
        request.y = 1.0;
        ROS_INFO("Teleport %s to x: %d, y: %d", m_strSelectedTurtle.c_str(), request.x, request.y);
        ros::service::call<turtlesim::TeleportAbsolute>(strServiceName, sTeleportAbsolute);
        auto response = sTeleportAbsolute.response;
        
    }


    void TurtlePlugin::on_selection_changed()
    {
        auto current = m_pUi->treeTurtles->currentItem(); // TODO use member list if multiple turtles are selected
        m_strSelectedTurtle = current->text(0).toStdString();
        ROS_INFO("Turtle %s selected", m_strSelectedTurtle.c_str());
    }


} // namespace

// Deprecated
// See: http://wiki.ros.org/pluginlib#pluginlib.2Fpluginlib_groovy.Simplified_Export_Macro
//PLUGINLIB_DECLARE_CLASS(rqt_turtle, TurtlePlugin, rqt_turtle::TurtlePlugin, rqt_gui_cpp::Plugin)
PLUGINLIB_EXPORT_CLASS(rqt_turtle::TurtlePlugin, rqt_gui_cpp::Plugin)