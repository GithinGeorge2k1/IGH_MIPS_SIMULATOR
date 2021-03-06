#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "Data.h"
#include "Maps.h"
#include "utilities.h"
#include "cacheconfig.h"

#include <QFile>
#include <QMessageBox>
#include <QFileDialog>
#include <QTextStream>
#include <QStringList>
#include <QDebug>

MainWindow* MainWindow::obj=NULL;
bool MainWindow::ValidCodePresent=false;
const int noOfTables=1;
MainWindow* MainWindow::getInstance()
{
    if(obj==NULL)
        return new MainWindow();
    return obj;
}
MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    fowd=new QCheckBox("Data Forwarding", this);
    fowd->setLayoutDirection(Qt::LeftToRight);
    connect(fowd, &QCheckBox::toggled, this, &MainWindow::ForwardingEnabled);
    ui->statusbar->addPermanentWidget(fowd);
    isExecuting=false;
    isTimeLineLocked=false;

    tableIndex=0;
    timeline=new QTableWidget* [noOfTables];
    timeline[tableIndex]=ui->timeline1;

    timeline[tableIndex]->setRowCount(1500);
    timeline[tableIndex]->setColumnCount(4500);

    stallList=ui->listWidget;
    Maps::getInstance();
    Data::getInstance();
    refreshAllPanels();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::refreshAllPanels(){
    Data* x=Data::getInstance();
    QString text=x->displayRegisters();
    ui->textBrowser->setHtml(text);
    text=x->displayData();
    ui->textBrowser_3->setPlainText(text);
    text=x->forConsole();
    ui->textBrowser_4->setHtml(text);
}

void MainWindow::setNewTable()
{
    tableIndex++;
    if(tableIndex>=noOfTables)
    {
        QMessageBox::warning(this, "MessageBox", "Exceeded Table Limit. Timeline Locked");
        isTimeLineLocked=true;
        return;
    }
    timeline[tableIndex]->clearContents();
}

int storeAllLabelsAndData(QTextStream& in){
    bool h1=false,h2=false,h3=false; //.text, .data, .globl main
    //LABEL SHOULD GET CORRECT INSTRUCTION NO(NOT TO BE CONFUSED WITH LINE NO)
    int instructionsize=0;

    int start=-1;
    Data* x=Data::getInstance();
    int lineNo=0;

    while(!in.atEnd()){
        lineNo++;
        QString text=in.readLine().simplified();
        if(text[0]=='#' || text=="")
              continue;
        if(text.indexOf('#')!=-1)
            text=text.section('#',0,0);
        //ADDING DATA TO THE DATA SECTION
        if(!h1 && text==".data"){
            QRegExp sep("(,| |, )");
            bool checkingDataInNextLine=true;
            while(checkingDataInNextLine){
                lineNo++;
                text=in.readLine().simplified();
                QStringList list=text.split(sep);
                if(list.at(0).indexOf(":")==list.at(0).length()-1){
                    x->variableMap[list.at(0).section(':',0,0)]=x->dataSize;
                }
                if(text.contains(".word")){
                    checkingDataInNextLine=true;
                    for(int i=0;i<list.length();i++){
                        if(isValue(list.at(i))){
                            x->data[x->dataSize]=convertToInt(list.at(i));
                            x->dataSize++;
                        }
                    }

                }
                if(!h2 && text==".text"){
                    h2=true;
                    checkingDataInNextLine=false;
                }
            }
            h1=true;
            continue;
        }

        if(!h3 && text==".globl main"){
            h3=true;
            start=lineNo;
            continue;
        }
        QStringList list=text.split(" ");
        if(list.at(0).indexOf(":")==list.at(0).length()-1){
            x->labelMap[list.at(0).section(':',0,0)]=instructionsize+2;
        }
        if(list.length()!=1)
            instructionsize++;

    }

    return start;
}

void MainWindow::on_actionReinitialize_and_Load_File_triggered()
{
    int start=-1;
    isExecuting=false;
    Data* D=Data::getInstance();
    initialize();

    CacheConfig popup;
    popup.setModal(true);
    popup.exec();
    if((!D->cache[0]->valid) || (!D->cache[1]->valid)){
        MainWindow::ValidCodePresent=false;
        D->cache[0]->valid=false;
        D->cache[1]->valid=false;
        QMessageBox::warning(this,"Cache Parameter Error","Please Set Cache Parameter as per standard");
        return;
    }

    QString path=QFileDialog::getOpenFileName(this,"Open File");
    QFile file(path);
    if(!file.open(QFile::ReadOnly | QFile::Text)){
        MainWindow::ValidCodePresent=false;
        D->cache[0]->valid=false;
        D->cache[1]->valid=false;
        return;
    }
    refreshAllPanels();
    QTextStream in(&file);
    bool lineValid;
    int lineNo=0;
    start=storeAllLabelsAndData(in);
    in.seek(0);


            //MODIFYING JAL AND SHOWING THEM ON TEXT TAB
    if(D->labelMap.contains("main")){
        D->instructions[0]=D->instructions[0] | D->labelMap["main"];
        ui->textBrowser_2->append(QString("0. [0x%2]  jal 0x%1").arg(D->labelMap["main"]).arg(decimalToHex(D->instructions[0])));
        ui->textBrowser_2->append(QString("1. [0x00000000]  nop"));
    }
    else
    {
        QMessageBox::warning(this,"Wrong Code","No Entry Point Defined!! Define Main");
        MainWindow::ValidCodePresent=false;
        file.close();
        return;
    }
            //..............................................//

    QString dataText=D->displayData();
    ui->textBrowser_3->setPlainText(dataText);
    QString tempLabel="";
    while(!in.atEnd()){
        lineNo++;
        QString text=in.readLine().simplified();
        if(lineNo<=start){
            continue;
        }
        if(text[0]=='#' || text=="")
              continue;
        if(text.indexOf('#')!=-1){
            text=text.section('#',0,0);
        }
        text=text.trimmed();
        lineValid=D->addCode(text);
        if(lineValid){
            if(!D->isLabel)
            {
                QString instructionHex=decimalToHex(D->instructions[D->instructionSize-1]);
                ui->textBrowser_2->append(QString("%2. [0x%3]  %1   %4").arg(text).arg(D->instructionSize-1).arg(instructionHex).arg(tempLabel));
                tempLabel="";
                D->assemblyInstruction.push_back(text);
            }
            else{
                tempLabel=text.section(':',0,0);
                tempLabel=QString("[%1]").arg(tempLabel);
            }
            MainWindow::ValidCodePresent=true;
        }
        else{
            QMessageBox::warning(this,"Invalid Assembly Code",QString("Syntax error found while parsing at line %1").arg(lineNo));
            MainWindow::ValidCodePresent=false;
            return;
        }
    }
    file.close();

}

void MainWindow::on_actionQuit_triggered()
{
    delete Data::getInstance();
    QApplication::quit();
}

void MainWindow::initialize(){
    Data* D=Data::getInstance();
    ui->textBrowser_2->setPlainText("");
    ui->textBrowser_3->setPlainText("");
    ui->listWidget->clear();
    D->initialize();
    timeline[0]->clear();
    refreshAllPanels();
    MainWindow::ValidCodePresent=false;

}

void MainWindow::on_actionClear_Registers_triggered()
{
    Data* D=Data::getInstance();
    memset(D->R,0,sizeof(D->R));
    ui->textBrowser->setHtml(D->displayRegisters());
}

void MainWindow::on_actionRun_triggered()
{
    if(MainWindow::ValidCodePresent){
        isExecuting=true;
        Data *D=Data::getInstance();
        if(!D->labelMap.contains("main")){
            QMessageBox::warning(this,"Cannot find main","No entry point defined");
            return;
        }
        bool isExitSmooth=D->run(timeline,stallList);
        refreshAllPanels();
        if(!isExitSmooth){
            QMessageBox::information(this,"Run Error",QString("No Valid Instruction at %1").arg(D->instructionSize+1));
        }
        else{

            QMessageBox::information(this,"Program Done","EXECUTION COMPLETED!!");
            D->nopOccured=false;
        }
        isExecuting=false;
    }
    else{
        QMessageBox::warning(this,"Invalid Assembly Code","Cannot Run!! Invalid Code");
        return;
    }
}

void MainWindow::on_actionRun_Step_By_Step_triggered()
{
    if(MainWindow::ValidCodePresent){
        isExecuting=true;
        Data *D=Data::getInstance();
        if(!D->labelMap.contains("main")){
            QMessageBox::warning(this,"Cannot find main","No entry point was defined");
            return;
        }
        bool isExitSmooth=D->runStepByStep(timeline,stallList);
        refreshAllPanels();
        if(!isExitSmooth){
            QMessageBox::information(this,"Run Error",QString("No Valid Instruction at %1").arg(D->instructionSize+1));
            isExecuting=false;
        }
        else if(D->nopOccured){
            QMessageBox::information(this,"Program Done","EXITED VIA NOP INSTRUCTION");
             D->nopOccured=false;
             isExecuting=false;
        }
    }
    else{
        QMessageBox::warning(this,"Invalid Assembly Code","Cannot Run!! Invalid Code");
        return;
    }
}

void MainWindow::on_actionHelp_triggered()
{
    helpwindow=new About_Help(nullptr);
    helpwindow->show();
}

void MainWindow::ForwardingEnabled()
{
    Data* D=Data::getInstance();
    if(isExecuting)// To prevent Data Forwarding to be enabled in between step by step execution of the Program | This option can be changed only before execution i.e. PC=0
    {
        ui->statusbar->showMessage("Cannot toggle option during Execution", 5000);
        if(D->FWD_ENABLED)
            fowd->setCheckState(Qt::Checked);
        else
            fowd->setCheckState(Qt::Unchecked);
        return;
    }
    D->FWD_ENABLED=!D->FWD_ENABLED;
    if(D->FWD_ENABLED)
        ui->statusbar->showMessage("Forwarding is Now Enabled", 5000);
    else
        ui->statusbar->showMessage("Forwarding is Now Disabled", 5000);
}
