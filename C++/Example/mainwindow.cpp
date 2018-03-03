#include "mainwindow.h"
#include "ui_mainwindow.h"



#include <QFileDialog>


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);



    // Start RoboDK API here (RoboDK will start if it is not running)
    ROBOT = NULL;
    RDK = new RoboDK();
    if (!RDK->Connected()){
        qDebug() << "Failed to start RoboDK API!!";
    }

}

MainWindow::~MainWindow() {
    delete ui;
    delete RDK;
}

bool MainWindow::Check_RoboDK(){
    if (RDK == NULL){
        statusBar()->showMessage("RoboDK API is not connected");
        return false;
    }
    if (!RDK->Connected()){
        statusBar()->showMessage("RoboDK is not running");
        return false;
    }
    return true;
}

bool MainWindow::Check_Robot(){
    if (!Check_RoboDK()){ return false; }

    if (ROBOT == NULL){
        statusBar()->showMessage("Select a robot first");
        return false;
    }
    if (!ROBOT->Valid()){
        statusBar()->showMessage("Robot item is not valid");
        return false;
    }
    return true;
}

void MainWindow::Select_Robot(){
    if (ROBOT != NULL){
        delete ROBOT;
        ROBOT = NULL;
    }
    ROBOT = new Item(RDK->ItemUserPick("Select a robot", RoboDK::ITEM_TYPE_ROBOT));
    //ROBOT = new Item(RDK->getItem("UR10", RoboDK::ITEM_TYPE_ROBOT));
    if (Check_Robot()){
        statusBar()->showMessage("Robot selected: " + ROBOT->Name());
    }
}

void MainWindow::on_btnLoadFile_clicked() {
    if (!Check_RoboDK()){ return; }

    QStringList files = QFileDialog::getOpenFileNames(this, tr("Open one or more files with RoboDK"));
    foreach (QString file, files){
        qDebug() << "Loading: " << file;
        RDK->AddFile(file);
    }

    if (!Check_Robot()){
        Select_Robot();
    }
}

void MainWindow::on_btnShowRoboDK_clicked(){
    if (!Check_RoboDK()){ return; }

    RDK->ShowRoboDK();
}

void MainWindow::on_btnHideRoboDK_clicked(){
    if (!Check_RoboDK()){ return; }

    RDK->HideRoboDK();
}

void MainWindow::on_btnSelectRobot_clicked(){
    Select_Robot();
}


void MainWindow::on_btnGetPosition_clicked(){
    if (!Check_Robot()){ return; }

    QString separator = " , ";
    int decimals = 1;

    // Get robot joints
    tJoints joints(ROBOT->Joints());
    QString joints_str = joints.ToString(separator, decimals);
    ui->txtJoints->setText(joints_str);

    // Get robot pose
    Mat robot_pose(ROBOT->Pose());
    QString pose_str = robot_pose.ToString(separator, decimals);
    ui->txtXYZWPR->setText(pose_str);
}

void MainWindow::on_btnMoveJoints_clicked(){
    if (!Check_Robot()){ return; }

    tJoints joints;

    joints.FromString(ui->txtJoints->text());

    bool blocking = true;

    ROBOT->MoveJ(joints, blocking);

}

void MainWindow::on_btnMovePose_clicked(){
    if (!Check_Robot()){ return; }

    Mat pose;

    pose.FromString(ui->txtXYZWPR->text());

    bool blocking = true;

    ROBOT->MoveJ(pose, blocking);

}

void MainWindow::on_btnProgRun_clicked(){
    if (!Check_Robot()){ return; }

    QString program_name = ui->txtProgName->text();

    RDK->RunProgram(program_name);
}

void MainWindow::on_btnTestButton_clicked(){
    if (!Check_Robot()){ return; }

/*
    if (!Check_Robot()) { return; }
    QList<Item> items;
    items.append(*ROBOT);
    items.append(RDK->getItemList(RoboDK::ITEM_TYPE_TARGET));
    qDebug() << "Neighbors are: \n";
    for (int i = 0; i < items.size(); i++)
    {
        qDebug() << items[i].Name() << ": \n";
        for (int j = 0; j < items.size(); j++) {
            if (i != j){
                if (ROBOT->MoveL_Test(items[i].Joints(), items[j].Joints(), 4) == 0){
                    Mat pose = ROBOT->SolveFK(items[j].Joints());
                } else if (ROBOT->MoveJ_Test(items[i].Joints(), items[j].Joints(), 4) == 0){

                }
                qDebug() << "\t\t" << items[j].Name();
            }
        }
    }
    return;
*/


    // Example to iterate through all the existing targets in the station (blocking):
    QList<Item> targets = RDK->getItemList(RoboDK::ITEM_TYPE_TARGET);
    foreach (Item target, targets){
        if (target.Type() == RoboDK::ITEM_TYPE_TARGET){
            ui->statusBar->showMessage("Moving to: " + target.Name());
            qApp->processEvents();
            ROBOT->MoveJ(target);
        }
    }




    return;

    QList<Item> list = RDK->getItemList();


    Mat pose_robot_base_abs = ROBOT->PoseAbs();
    Mat pose_robot = ROBOT->Pose();
    Mat pose_tcp = ROBOT->PoseTool();

    qDebug() << "Absolute Position of the robot:";
    qDebug() << pose_robot_base_abs;

    qDebug() << "Current robot position (active tool with respect to the active reference):";
    qDebug() << pose_robot;

    qDebug() << "Position of the active TCP:";
    qDebug() << pose_tcp;

    QList<Item> tool_list = ROBOT->Childs();
    if (tool_list.length() <= 0){
        statusBar()->showMessage("No tools available for the robot " + ROBOT->Name());
        return;
    }

    Item tool = tool_list.at(0);
    qDebug() << "Using tool: " << tool.Name();

    Mat pose_robot_flange_abs = tool.PoseAbs();
    pose_tcp = tool.PoseTool();
    Mat pose_tcp_abs = pose_robot_flange_abs * pose_tcp;


    Item object = RDK->getItem("", RoboDK::ITEM_TYPE_FRAME);
    Mat pose_object_abs = object.PoseAbs();
    qDebug() << pose_tcp;

    Mat tcp_wrt_obj = pose_object_abs.inverted() * pose_tcp_abs;

    qDebug() << "Pose of the TCP with respect to the selected reference frame";
    qDebug() << tcp_wrt_obj;

    tXYZWPR xyzwpr;
    tcp_wrt_obj.ToXYZRPW(xyzwpr);

    this->statusBar()->showMessage(QString("Tool with respect to %1").arg(object.Name()) + QString(": [X,Y,Z,W,P,R]=[%1, %2, %3, %4, %5, %6] mm/deg").arg(xyzwpr[0],0,'f',3).arg(xyzwpr[1],0,'f',3).arg(xyzwpr[2],0,'f',3).arg(xyzwpr[3],0,'f',3).arg(xyzwpr[4],0,'f',3).arg(xyzwpr[5],0,'f',3) );

}

void MainWindow::on_btnTXn_clicked(){ IncrementalMove(0, -1); }
void MainWindow::on_btnTYn_clicked(){ IncrementalMove(1, -1); }
void MainWindow::on_btnTZn_clicked(){ IncrementalMove(2, -1); }
void MainWindow::on_btnRXn_clicked(){ IncrementalMove(3, -1); }
void MainWindow::on_btnRYn_clicked(){ IncrementalMove(4, -1); }
void MainWindow::on_btnRZn_clicked(){ IncrementalMove(5, -1); }

void MainWindow::on_btnTXp_clicked(){ IncrementalMove(0, +1); }
void MainWindow::on_btnTYp_clicked(){ IncrementalMove(1, +1); }
void MainWindow::on_btnTZp_clicked(){ IncrementalMove(2, +1); }
void MainWindow::on_btnRXp_clicked(){ IncrementalMove(3, +1); }
void MainWindow::on_btnRYp_clicked(){ IncrementalMove(4, +1); }
void MainWindow::on_btnRZp_clicked(){ IncrementalMove(5, +1); }

void MainWindow::IncrementalMove(int id, double sense){
    if (!Check_Robot()) { return; }

    // check the index
    if (id < 0 || id >= 6){
        qDebug()<< "Invalid id provided to for an incremental move";
        return;
    }

    // calculate the relative movement
    double step = sense * ui->spnStep->value();

    // apply to XYZWPR
    tXYZWPR xyzwpr;
    for (int i=0; i<6; i++){
        xyzwpr[i] = 0;
    }
    xyzwpr[id] = step;

    Mat pose_increment;
    pose_increment.FromXYZRPW(xyzwpr);

    Mat pose_robot = ROBOT->Pose();

    Mat pose_robot_new;

    // apply relative to the TCP:
    pose_robot_new = pose_robot * pose_increment;

    ROBOT->MoveJ(pose_robot_new);

}




