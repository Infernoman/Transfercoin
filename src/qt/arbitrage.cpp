#include "arbitrage.h"
#include "ui_arbitrage.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include <qmessagebox.h>
#include <qtimer.h>
#include <rpcserver.h>

#include <QAbstractButton>
#include <QClipboard>
#include <QDebug>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>

#include <QUrl>
#include <QUrlQuery>
#include <QVariant>
#include <QJsonValue>
#include <QJsonDocument>
#include <QJsonObject>
#include <QVariantMap>
#include <QJsonArray>
#include <QTime>

#include <openssl/hmac.h>
#include <stdlib.h>

using namespace std;

Arbitrage::Arbitrage(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Arbitrage),
    model(0)
{
    ui->setupUi(this);
    timerid = 0;
    arbtimerid = 0;
    qDebug() <<  "Expected this";

    // Bittrex Balance page
    ui->BittrexBTCBalance->setTextFormat(Qt::RichText);ui->BittrexBTCAvailable->setTextFormat(Qt::RichText);ui->BittrexBTCPending->setTextFormat(Qt::RichText);
    ui->BittrexTXBalance->setTextFormat(Qt::RichText);ui->BittrexTXAvailable->setTextFormat(Qt::RichText);ui->BittrexTXPending->setTextFormat(Qt::RichText);
    // Bittrex Balance page

    // SafeCex Balance Page
    ui->SafecexBTCBalance->setTextFormat(Qt::RichText);ui->SafecexBTCAvailable->setTextFormat(Qt::RichText);ui->SafecexBTCPending->setTextFormat(Qt::RichText);
    ui->SafecexTXBalance->setTextFormat(Qt::RichText);ui->SafecexTXAvailable->setTextFormat(Qt::RichText);ui->SafecexTXPending->setTextFormat(Qt::RichText);
    // SafeCex Balance page

    // Extrade Balance Page
    ui->ExtradeBTCBalance->setTextFormat(Qt::RichText);ui->ExtradeBTCAvailable->setTextFormat(Qt::RichText);ui->ExtradeBTCPending->setTextFormat(Qt::RichText);
    ui->ExtradeTXBalance->setTextFormat(Qt::RichText);ui->ExtradeTXAvailable->setTextFormat(Qt::RichText);ui->ExtradeTXPending->setTextFormat(Qt::RichText);
    // Extrade Balance page

    //Set tabs to inactive
    ui->ArbitrageTabWidget->setTabEnabled(0,false);
    ui->ArbitrageTabWidget->setTabEnabled(1,false);
    ui->ArbitrageTabWidget->setTabEnabled(2,false);
    ui->ArbitrageTabWidget->setTabEnabled(3,false);
    ui->ArbitrageTabWidget->setTabEnabled(4,false);
    ui->ArbitrageTabWidget->setTabEnabled(6,false);
    ui->BittrexLabel->setVisible(false);
    ui->BittrexCheckbox->setVisible(false);
    ui->SafecexLabel->setVisible(false);
    ui->SafecexCheckbox->setVisible(false);
    ui->ExtradeLabel->setVisible(false);
    ui->ExtradeCheckbox->setVisible(false);
    ui->YobitLabel->setVisible(false);
    ui->YobitCheckbox->setVisible(false);
    // Set tabs to inactive

    // Listen for keypress
    connect(ui->PasswordInput, SIGNAL(returnPressed()),ui->LoadKeys,SIGNAL(clicked()));
    connect(ui->BittrexCheckbox, SIGNAL(clicked(bool)), this, SLOT(BittrexToggled(bool)));
    connect(ui->SafecexCheckbox, SIGNAL(clicked(bool)), this, SLOT(SafecexToggled(bool)));
    connect(ui->ExtradeCheckbox, SIGNAL(clicked(bool)), this, SLOT(ExtradeToggled(bool)));
    // Listen for keypress


    /* Debug Table Init */
    ui->DebugHistory->setRowCount(0);
    ui->DebugHistory->setColumnCount(1);
    ui->DebugHistory->verticalHeader()->setVisible(false);
    ui->DebugHistory->horizontalHeader()->setVisible(false);
    int Cellwidth =  ui->DebugHistory->width();
    ui->DebugHistory->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->DebugHistory->horizontalHeader()->resizeSection(1,Cellwidth);
    ui->DebugHistory->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->DebugHistory->horizontalHeader()->setStyleSheet("QHeaderView::section, QHeaderView::section * {font-weight :bold;}");
    /* Debug Table Init */

    /*Successful Trades Table Init*/
    ui->ArbHistoryTable->setColumnCount(7);
    ui->ArbHistoryTable->verticalHeader()->setVisible(false);
    ui->ArbHistoryTable->setHorizontalHeaderLabels(QStringList() << "Date" << "Exchange" << "OrderType" << "QTY" << "PricePerUnit" << "Total" << "Btc Gained");
    ui->ArbHistoryTable->setRowCount(0);
    Cellwidth = ui->ArbHistoryTable->width() / 7;
    ui->ArbHistoryTable->horizontalHeader()->setResizeMode(QHeaderView::Stretch);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(1,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(2,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(3,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(4,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(5,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(6,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->resizeSection(7,Cellwidth);
    ui->ArbHistoryTable->horizontalHeader()->setDefaultAlignment(Qt::AlignLeft);
    ui->ArbHistoryTable->horizontalHeader()->setStyleSheet("QHeaderView::section, QHeaderView::section * {font-weight :bold;}");
    /*Account History Table Init*/
}

void Arbitrage::BittrexToggled(bool clicked)
{
    if (clicked)
        this->EnableBittrex = true;
}
void Arbitrage::SafecexToggled(bool clicked)
{
    if (clicked)
        this->EnableSafecex = true;
}
void Arbitrage::ExtradeToggled(bool clicked)
{
    if (clicked)
        this->EnableExtrade = true;
}
void Arbitrage::YobitToggled(bool clicked)
{
    if (clicked)
        this->EnableYobit = true;
}

void Arbitrage::InitArbitrage()
{// todo - add internet connection/socket error checking.

        //Get default exchange info for the qlabels
        UpdaterFunction();
        qDebug() << "Updater called";
      if(this->timerid == 0)
        {
          //Timer is not set,lets create one.
          this->timer = new QTimer(this);
          connect(timer, SIGNAL(timeout()), this, SLOT(UpdaterFunction()));
          this->timer->start(5000);
          this->timerid = this->timer->timerId();
        }

}
void Arbitrage::on_ArbitrageTabWidget_tabBarClicked(int index)
{// tab was clicked, interrupt the timer and restart after action completed.

    this->timer->stop();
    LogPrintf("Arbitrage::on_ArbitrageTabWidget_tabBarClicked - index=%d\n", index);

    ActionsOnSwitch(index);

    this->timer->start();
}
void Arbitrage::UpdaterFunction()
{

     //int Retval = SetExchangeInfoTextLabels();

     ActionsOnSwitch(-1);
}
void Arbitrage::DisplayBalance(QLabel &BalanceLabel, QLabel &Available, QLabel &Pending, QString Currency, QString Response)
{// update display labels

    QString str;

    BalanceLabel.setTextFormat(Qt::RichText);
    Available.setTextFormat(Qt::RichText);
    Pending.setTextFormat(Qt::RichText);

    //Set the labels, parse the json result to get values.
    QJsonObject ResultObject = GetResultObjectFromJSONObject(Response);

    BalanceLabel.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( ResultObject["Balance"].toDouble(),'i',8) + "</span> " + Currency);
    Available.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( ResultObject["Available"].toDouble(),'i',8) + "</span> " +Currency);
    Pending.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( ResultObject["Pending"].toDouble(),'i',8) + "</span> " +Currency);
}
void Arbitrage::DisplaySafecexBalance(QLabel &BalanceLabel, QLabel &Available, QLabel &Pending, QString Currency, QString Response)
{// update display labels

    QString str;

    BalanceLabel.setTextFormat(Qt::RichText);
    Available.setTextFormat(Qt::RichText);
    Pending.setTextFormat(Qt::RichText);


    //Set the labels, parse the json result to get values.
    QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    BalanceLabel.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( ResponseObject.value("total").toDouble(),'i',8) + "</span> " + Currency);
    Available.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( ResponseObject.value("balance").toDouble(),'i',8) + "</span> " +Currency);
    Pending.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number(ResponseObject.value("pending").toDouble(),'i',8) + "</span> " +Currency);
}
void Arbitrage::DisplayYobitBalance(QLabel &BalanceLabel, QString Currency, QString Response)
{// update display labels

    QString str;

    BalanceLabel.setTextFormat(Qt::RichText);


    //Set the labels, parse the json result to get values.
    QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    BalanceLabel.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( ResponseObject.value("return").toObject().value("funds").toObject().value(Currency).toDouble(),'i',8) + "</span> " + Currency);
}
void Arbitrage::DisplayExtradeBalance(QLabel &BalanceLabel, QLabel &Available, QLabel &Pending, QString Currency, QString Response)
{// update display labels

    QString str;

    BalanceLabel.setTextFormat(Qt::RichText);
    Available.setTextFormat(Qt::RichText);
    Pending.setTextFormat(Qt::RichText);

    //Set the labels, parse the json result to get values.
    QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    double balance = ResponseObject.value(QString("available["+Currency+"]")).toDouble();
    double available = ResponseObject.value(QString("available["+Currency+"]")).toDouble();
    double pending = ResponseObject.value(QString("available["+Currency+"]")).toDouble();

    BalanceLabel.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( balance,'i',8) + "</span> " + Currency);
    Available.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( available,'i',8) + "</span> " +Currency);
    Pending.setText("<span style='font-weight:bold; font-size:11px; color:green'>" + str.number( pending,'i',8) + "</span> " +Currency);
}
void Arbitrage::ActionsOnSwitch(int index = -1)
{

    QString Response = "";

    if(index == -1){
       index = ui->ArbitrageTabWidget->currentIndex();
    }

    switch (index){
                case 0: // Successful Trades

                break;

                case 1: // Bittrex Balance
                    Response = GetBittrexBalance("BTC");
                    if(Response.size() > 0 && Response != "Error"){
                        DisplayBalance(*ui->BittrexBTCBalance,*ui->BittrexBTCAvailable,*ui->BittrexBTCPending, QString::fromUtf8("BTC"),Response);
                    }

                    Response = GetBittrexBalance("TX");
                    if(Response.size() > 0 && Response != "Error"){
                        DisplayBalance(*ui->BittrexTXBalance,*ui->BittrexTXAvailable,*ui->BittrexTXPending, QString::fromUtf8("TX"),Response);
                    }
                break;

                case 2: // Safecex Balance
                    Response = GetSafecexBalance("BTC");
                    if(Response.size() > 0 && Response != "Error"){
                        DisplaySafecexBalance(*ui->SafecexBTCBalance,*ui->SafecexBTCAvailable,*ui->SafecexBTCPending, QString::fromUtf8("BTC"),Response);
                    }

                    Response = GetSafecexBalance("TX");
                    if(Response.size() > 0 && Response != "Error"){
                        DisplaySafecexBalance(*ui->SafecexTXBalance,*ui->SafecexTXAvailable,*ui->SafecexTXPending, QString::fromUtf8("TX"),Response);
                    }
                break;

                case 3: // Extrade Balance
                    Response = GetExtradeBalance("BTC");
                    if(Response.size() > 0 && Response != "Error"){
                        DisplayExtradeBalance(*ui->ExtradeBTCBalance,*ui->ExtradeBTCAvailable,*ui->ExtradeBTCPending, QString::fromUtf8("BTC"),Response);
                    }

                    Response = GetExtradeBalance("TX");
                    if(Response.size() > 0 && Response != "Error"){
                        DisplayExtradeBalance(*ui->ExtradeTXBalance,*ui->ExtradeTXAvailable,*ui->ExtradeTXPending, QString::fromUtf8("TX"),Response);
                    }
                break;

                case 4: // Yobit Balance
                    Response = GetYobitBalance();
                    if(Response.size() > 0 && Response != "Error"){
                        DisplayYobitBalance(*ui->YobitBTCBalance,QString::fromUtf8("btc"),Response);
                    }

                    Response = GetYobitBalance();
                    if(Response.size() > 0 && Response != "Error"){
                        DisplayYobitBalance(*ui->YobitTXBalance,QString::fromUtf8("tx"),Response);
                    }
                break;

                case 5: // Api keys tab

                break;

                case 6: // Show Trade tab
                    QString str = "";
                    ui->TotalEarned->setText(str.number(this->TotalEarned, 'i', 8));
                break;

              }
}

//-------------------------------------------------//
//------------------   ApiKeys   ------------------//
//-------------------------------------------------//
void Arbitrage::on_UpdateKeys_clicked(bool Save, bool Load)
{
    this->EnableBittrex = false;
    this->EnableSafecex = false;
    this->EnableExtrade = false;
    bool disableBittrex = false;
    int total = 0;
    int WalletCheck = 0;
    QJsonDocument BjsonResponse;
    QJsonObject BResponseObject;
    QJsonDocument SjsonResponse;
    QJsonObject SResponseObject;
    QJsonDocument EjsonResponse;
    QJsonObject EResponseObject;
    QJsonDocument YjsonResponse;
    QJsonObject YResponseObject;
    int64_t BalanceCheck = pwalletMain->GetBalance();
    double Balance = ((double)BalanceCheck / (double)100000000);
    if (Balance < (double)10000) {
        QMessageBox::information(this,"Error","To enable more than 2 exchanges it requires a balance of more than 10 000 coins !");
        disableBittrex = true;
        WalletCheck = 2;
    } else if (Balance < (double)20000) {
        QMessageBox::information(this,"Error","To enable more than 3 exchanges it requires a balance of more than 20 000 coins !");
        disableBittrex = true;
        WalletCheck = 3;
    } else {
        disableBittrex = false;
        WalletCheck = 4;
    }
    WalletCheck = 4;
    disableBittrex = false;

    if (disableBittrex == false){
        if (ui->BittrexApiKeyInput->text() != "" && ui->BittrexSecretKeyInput->text() != ""){
            this->BittrexApiKey    = ui->BittrexApiKeyInput->text();
            this->BittrexSecretKey = ui->BittrexSecretKeyInput->text();
            BjsonResponse = QJsonDocument::fromJson(GetBittrexBalance("TX").toUtf8());          //get json from str.
            BResponseObject = BjsonResponse.object();
        }
    }
    if (ui->SafecexApiKeyInput->text() != "" && ui->SafecexSecretKeyInput->text() != ""){
        this->SafecexApiKey    = ui->SafecexApiKeyInput->text();
        this->SafecexSecretKey = ui->SafecexSecretKeyInput->text();
        SjsonResponse = QJsonDocument::fromJson(GetSafecexTXAddress().toUtf8());          //get json from str.
        SResponseObject = SjsonResponse.object();                              //get json obj
    }
    //if (ui->ExtradeApiKeyInput->text() != "" && ui->ExtradeSecretKeyInput->text() != ""){
        this->ExtradeApiKey    = ui->ExtradeApiKeyInput->text();
        this->ExtradeSecretKey = ui->ExtradeSecretKeyInput->text();
        EjsonResponse = QJsonDocument::fromJson(GetExtradeBalance("TX").toUtf8()); //get json from str.
        EResponseObject = EjsonResponse.object();                                 //get json obj
    //}
    if (ui->YobitApiKeyInput->text() != "" && ui->YobitSecretKeyInput->text() != ""){
        this->YobitApiKey    = ui->YobitApiKeyInput->text();
        this->YobitSecretKey = ui->YobitSecretKeyInput->text();
        YjsonResponse = QJsonDocument::fromJson(GetYobitBalance().toUtf8()); //get json from str.
        YResponseObject = YjsonResponse.object();                                 //get json obj
    }

    QString test2 = SResponseObject.value("deposit").toString();
    if (test2 != "" && total <= WalletCheck) {
        total++;
        ui->SafecexApiKeyInput->setEchoMode(QLineEdit::Password);
        ui->SafecexSecretKeyInput->setEchoMode(QLineEdit::Password);
        ui->ArbitrageTabWidget->setTabEnabled(2,true);
        ui->PasswordInput->setText("");
        ui->SafecexLabel->setVisible(true);
        ui->SafecexCheckbox->setVisible(true);
    }
    QJsonValue test3 = EResponseObject.value("errors");
    if ((test3.isUndefined() || test3.isNull()) && total <= WalletCheck) {
        total++;
        ui->ExtradeApiKeyInput->setEchoMode(QLineEdit::Password);
        ui->ExtradeSecretKeyInput->setEchoMode(QLineEdit::Password);
        ui->ArbitrageTabWidget->setTabEnabled(3,true);
        ui->PasswordInput->setText("");
        ui->ExtradeLabel->setVisible(true);
        ui->ExtradeCheckbox->setVisible(true);
    }
    bool test4 = YResponseObject.value("success").toBool();
    if (test4 == true && total <= WalletCheck) {
        total++;
        ui->YobitApiKeyInput->setEchoMode(QLineEdit::Password);
        ui->YobitSecretKeyInput->setEchoMode(QLineEdit::Password);
        ui->ArbitrageTabWidget->setTabEnabled(4,true);
        ui->PasswordInput->setText("");
        ui->YobitLabel->setVisible(true);
        ui->YobitCheckbox->setVisible(true);
    }
    if (BResponseObject.value("success").toBool() == true && total <= WalletCheck && disableBittrex == false) {
        total++;
        ui->BittrexApiKeyInput->setEchoMode(QLineEdit::Password);
        ui->BittrexSecretKeyInput->setEchoMode(QLineEdit::Password);
        ui->ArbitrageTabWidget->setTabEnabled(1,true);
        ui->PasswordInput->setText("");
        ui->BittrexLabel->setVisible(true);
        ui->BittrexCheckbox->setVisible(true);
    }

    if ( total < 2){
        QMessageBox::information(this,"API Configuration Failed","Api configuration was unsuccesful.");
    } else if ( total >= 2 && Load){
        QMessageBox::information(this,"API Configuration Complete","Your API keys have been loaded and the connection has been successfully configured and tested.");
        ui->ArbitrageTabWidget->setTabEnabled(0,true);
        ui->ArbitrageTabWidget->setTabEnabled(6,true);
    } else if ( total >= 2 && Save){
        QMessageBox::information(this,"API Configuration Complete","Your API keys have been saved and the connection has been successfully configured and tested.");
        ui->ArbitrageTabWidget->setTabEnabled(0,true);
        ui->ArbitrageTabWidget->setTabEnabled(6,true);
    } else if ( total >= 2){
        QMessageBox::information(this,"API Configuration Complete","Api connection has been successfully configured and tested.");
        ui->ArbitrageTabWidget->setTabEnabled(0,true);
        ui->ArbitrageTabWidget->setTabEnabled(6,true);
    }
}
void Arbitrage::on_SaveKeys_clicked()
{
    bool fSuccess = true;
    boost::filesystem::path pathConfigFile = GetDataDir() / "ARBcache";
    boost::filesystem::ofstream stream (pathConfigFile.string(), ios::out | ios::trunc);

    // Qstring to string
    string password = ui->PasswordInput->text().toUtf8().constData();

    if (password.length() <= 6){
        QMessageBox::information(this,"Error !","Your password is too short !");
        fSuccess = false;
        stream.close();
    }

    // qstrings to utf8, add to byteArray and convert to const char for stream
    string BittrexSecret = ui->BittrexSecretKeyInput->text().toUtf8().constData();
    string BittrexKey = ui->BittrexApiKeyInput->text().toUtf8().constData();
    string SafecexSecret = ui->SafecexSecretKeyInput->text().toUtf8().constData();
    string SafecexKey = ui->SafecexApiKeyInput->text().toUtf8().constData();
    string ExtradeSecret = ui->ExtradeSecretKeyInput->text().toUtf8().constData();
    string ExtradeKey = ui->ExtradeApiKeyInput->text().toUtf8().constData();
    string YobitSecret = ui->YobitSecretKeyInput->text().toUtf8().constData();
    string YobitKey = ui->YobitApiKeyInput->text().toUtf8().constData();
    string BSecret = "";
    string BKey = "";
    string SSecret = "";
    string SKey = "";
    string ESecret = "";
    string EKey = "";
    string YSecret = "";
    string YKey = "";


    if (stream.is_open() && fSuccess)
    {
        BSecret = encryptDecrypt(BittrexSecret, password);
        BKey = encryptDecrypt(BittrexKey, password);
        SSecret = encryptDecrypt(SafecexSecret, password);
        SKey = encryptDecrypt(SafecexKey, password);
        ESecret = encryptDecrypt(ExtradeSecret, password);
        EKey = encryptDecrypt(ExtradeKey, password);
        YSecret = encryptDecrypt(YobitSecret, password);
        YKey = encryptDecrypt(YobitKey, password);
        stream << BSecret << '\n';
        stream << BKey << '\n';
        stream << SSecret << '\n';
        stream << SKey << '\n';
        stream << ESecret << '\n';
        stream << EKey << '\n';
        stream << YSecret << '\n';
        stream << YKey;
        stream.close();
    }
    if (fSuccess) {
        bool Save = true;
        on_UpdateKeys_clicked(Save);
    }
}
void Arbitrage::on_LoadKeys_clicked()
{
    bool fSuccess = true;
    boost::filesystem::path pathConfigFile = GetDataDir() / "ARBcache";
    boost::filesystem::ifstream stream (pathConfigFile.string());

    // Qstring to string
    string password = ui->PasswordInput->text().toUtf8().constData();

    if (password.length() <= 6){
        QMessageBox::information(this,"Error !","Your password is too short !");
        fSuccess = false;
        stream.close();
    }

    QString BSecret = "";
    QString BKey = "";
    QString SSecret = "";
    QString SKey = "";
    QString ESecret = "";
    QString EKey = "";
    QString YSecret = "";
    QString YKey = "";

    if (stream.is_open() && fSuccess)
    {
        int i =0;
        for ( std::string line; std::getline(stream,line); )
        {
            if (i == 0 ){
                BSecret = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->BittrexSecretKeyInput->setText(BSecret);
            } else if (i == 1){
                BKey = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->BittrexApiKeyInput->setText(BKey);
            } else if (i == 2) {
                SSecret = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->SafecexSecretKeyInput->setText(SSecret);
            } else if (i == 3) {
                SKey = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->SafecexApiKeyInput->setText(SKey);
            } else if (i == 4) {
                ESecret = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->ExtradeSecretKeyInput->setText(ESecret);
            } else if (i == 5) {
                EKey = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->ExtradeApiKeyInput->setText(EKey);
            } else if (i == 6) {
                YSecret = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->YobitSecretKeyInput->setText(YSecret);
            } else if (i == 7) {
                YKey = QString::fromUtf8(encryptDecrypt(line, password).c_str());
                ui->YobitApiKeyInput->setText(YKey);
            }
            i++;
        }
        stream.close();
    }
    if (fSuccess) {
        bool Save = false;
        bool Load = true;
        on_UpdateKeys_clicked(Save, Load);
    }
}
string Arbitrage::encryptDecrypt(string toEncrypt, string password)
{

    char * key = new char [password.size()+1];
    std::strcpy (key, password.c_str());
    key[password.size()] = '\0'; // don't forget the terminating 0

    string output = toEncrypt;

    for (unsigned int i = 0; i < toEncrypt.size(); i++)
        output[i] = toEncrypt[i] ^ key[i % (sizeof(key) / sizeof(char))];
    return output;
}
//-------------------------------------------------//
//-----------------^   ApiKeys   ^-----------------//
//-------------------------------------------------//





//-------------------------------------------------//
//------------------   Bittrex   ------------------//
//-------------------------------------------------//
void Arbitrage::on_BittrexTXGenDepositBTN_clicked()
{
    QString response         =  GetBittrexTXAddress();
    QJsonObject ResultObject =  GetResultObjectFromJSONObject(response);
    ui->BittrexTXAddress->setText(ResultObject["Address"].toString());
}
void Arbitrage::on_BittrexBTCGenDepositBTN_clicked()
{
    QString response         =  GetBittrexBTCAddress();
    QJsonObject ResultObject =  GetResultObjectFromJSONObject(response);
    ui->BittrexBTCAddress->setText(ResultObject["Address"].toString());
}
void Arbitrage::on_Bittrex_Withdraw_MaxTX_Amount_clicked()
{
    QString response = GetBittrexBalance("TX");
    QString str;

    QJsonObject ResultObject =  GetResultObjectFromJSONObject(response);

    double AvailableTX = ResultObject["Available"].toDouble();

    ui->BittrexWithdrawTXInput->setText(str.number(AvailableTX,'i',8));
}
void Arbitrage::on_Bittrex_Withdraw_MaxBTC_Amount_clicked()
{
    QString response = GetBittrexBalance("BTC");
    QString str;

    QJsonObject ResultObject =  GetResultObjectFromJSONObject(response);

    double AvailableBTC = ResultObject["Available"].toDouble();

    ui->BittrexWithdrawBTCInput->setText(str.number(AvailableBTC,'i',8));
}
void Arbitrage::on_BittrexWithdrawTXBtn_clicked()
{
    double Quantity = ui->BittrexWithdrawTXInput->text().toDouble();
    QString Qstr;
    QString Coin = "TX";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.02),'i',8);
            Msg += " TX to ";
            Msg += ui->BittrexWithdrawTXAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response =  BittrexWithdraw(Quantity, ui->BittrexWithdrawTXAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["message"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
void Arbitrage::on_BittrexWithdrawBTCBtn_clicked()
{
    double Quantity = ui->BittrexWithdrawBTCInput->text().toDouble();
    QString Qstr;
    QString Coin = "BTC";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.0002),'i',8);
            Msg += " BTC to ";
            Msg += ui->BittrexWithdrawBTCAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response =  BittrexWithdraw(Quantity, ui->BittrexWithdrawBTCAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["message"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
QString Arbitrage::BuyTXBittrex(double Quantity, double Rate)
{

    QString str = "";
    QString URL = "https://bittrex.com/api/v1.1/market/";
            URL += "buylimit";
            URL += "?apikey=";
            URL += this->BittrexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&market=BTC-TX&quantity=";
            URL += str.number(Quantity,'i',8);
            URL += "&rate=";
            URL += str.number(Rate,'i',8);

    QString Response = sendBittrexRequest(URL);
    return Response;
}
void Arbitrage::BittrexBuy_Extrade(QJsonObject bittrex, QJsonObject extrade)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double extradeQ = extrade.value(QString("order-book")).toObject().value("bid").toArray().first().toObject().value("order_amount").toString().toDouble();
    double price = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();

    if (extradeQ > bittrexQ) {
        double amount = bittrexQ;

        QString Response = BuyTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from ExTrade";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, false);
        } else {
            QString update = "ExtradeBuy - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = extradeQ;

        QString Response = BuyTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Extrade";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, false);
        } else {
            QString update = "BittrexBuy - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
void Arbitrage::BittrexBuy_Safecex(QJsonObject bittrex, QJsonObject safecex)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double safecexQ = safecex.value("bids").toArray().first().toObject().value("amount").toString().toDouble();
    double price = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();

    if (safecexQ > bittrexQ) {
        double amount = bittrexQ;

        QString Response = BuyTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, false);
        } else {
            QString update = "BittrexBuy_Safecex - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = safecexQ;

        QString Response = BuyTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, false);
        } else {
            QString update = "BittrexBuy_Safecex - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
void Arbitrage::BittrexBuy_Yobit(QJsonObject bittrex, QJsonObject yobit)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double yobitQ = yobit.value("bids").toArray().first().toArray().last().toDouble();
    double price = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();

    if (yobitQ > bittrexQ) {
        double amount = bittrexQ;

        QString Response = BuyTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, false);
        } else {
            QString update = "BittrexBuy_Yobit - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = yobitQ;

        QString Response = BuyTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, false);
        } else {
            QString update = "BittrexBuy_Yobit - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::SellTXBittrex(double Quantity, double Rate)
{

    QString str = "";
    QString URL = "https://bittrex.com/api/v1.1/market/";
            URL += "selllimit";
            URL += "?apikey=";
            URL += this->BittrexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&market=BTC-TX&quantity=";
            URL += str.number(Quantity,'i',8);
            URL += "&rate=";
            URL += str.number(Rate,'i',8);

    QString Response = sendBittrexRequest(URL);
    return Response;
}
/* UNFINISHED
void Arbitrage::BittrexSell_Extrade(QJsonObject bittrex, QJsonObject extrade)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Quantity").toDouble();
    double extradeQ = extrade.value("bids").toArray().first().toObject().value("amount").toString().toDouble();
    double price = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();

    if (bittrexQ > extradeQ) {
        double amount = extradeQ;

        QString Response = SellTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, true);
        } else {
            QString update = "BittrexSell_Extrade - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = bittrexQ;

        QString Response = SellTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, true);
        } else {
            QString update = "BittrexSell_Extrade - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
*/
void Arbitrage::BittrexSell_Safecex(QJsonObject bittrex, QJsonObject safecex)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Quantity").toDouble();
    double safecexQ = safecex.value("bids").toArray().first().toObject().value("amount").toString().toDouble();
    double price = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();

    if (bittrexQ > safecexQ) {
        double amount = safecexQ;

        QString Response = SellTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, true);
        } else {
            QString update = "BittrexSell_Safecex - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = bittrexQ;

        QString Response = SellTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, true);
        } else {
            QString update = "BittrexSell_Safecex - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
void Arbitrage::BittrexSell_Yobit(QJsonObject bittrex, QJsonObject yobit)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Quantity").toDouble();
    double yobitQ = yobit.value("bids").toArray().first().toArray().last().toDouble();
    double price = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();

    if (bittrexQ > yobitQ) {
        double amount = yobitQ;

        QString Response = SellTXBittrex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, true);
        } else {
            QString update = "BittrexSell_Yobit - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = bittrexQ;

        QString Response = SellTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Bittrex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Bittrex", amount, price, true);
        } else {
            QString update = "BittrexSell_Yobit - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::BittrexWithdraw(double Amount, QString Address, QString Coin)
{

    QString str = "";
    QString URL = "https://bittrex.com/api/v1.1/account/withdraw?apikey=";
            URL += this->BittrexApiKey;
            URL += "&currency=";
            URL += Coin;
            URL += "&quantity=";
            URL += str.number(Amount,'i',8);
            URL += "&address=";
            URL += Address;
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendBittrexRequest(URL);
     return Response;
}
QString Arbitrage::GetBittrexOrderBook()
{

    QString  Response = sendBittrexRequest("https://bittrex.com/api/v1.1/public/getorderbook?market=BTC-TX&type=both&depth=50", false);
    return Response;
}
QString Arbitrage::GetBittrexBalance(QString Currency)
{

    QString URL = "https://bittrex.com/api/v1.1/account/getbalance?apikey=";
            URL += this->BittrexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&currency=";
            URL += Currency;

    QString Response = sendBittrexRequest(URL);
     return Response;
}
QString Arbitrage::GetBittrexTXAddress()
{

    QString URL = "https://bittrex.com/api/v1.1/account/getdepositaddress?apikey=";
            URL += this->BittrexApiKey;
            URL += "&nonce=12345434&currency=TX";

    QString Response = sendBittrexRequest(URL);
    return Response;
}
QString Arbitrage::GetBittrexBTCAddress()
{

    QString URL = "https://bittrex.com/api/v1.1/account/getdepositaddress?apikey=";
            URL += this->BittrexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&currency=BTC";

    QString Response = sendBittrexRequest(URL);
    return Response;
}
QString Arbitrage::sendBittrexRequest(QString url, bool post)
{

    QString Response = "";
    QString Secret   = this->BittrexSecretKey;

    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req = QNetworkRequest(QUrl(url));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //make this conditional,depending if we are using private api call
    if (post) {
        req.setRawHeader("apisign",HMAC_SHA512_SIGNER(url,Secret).toStdString().c_str()); //set header for bittrex
    }

    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        //success
        Response = reply->readAll();
        //QMessageBox::information(this,"Error",Response);
        delete reply;
    }
    else{
        //failure
        qDebug() << "Failure" <<reply->errorString();
        Response = "Error";
        QMessageBox::information(this,"Error",reply->errorString());
        delete reply;
        }

     return Response;
}
//-------------------------------------------------//
//-----------------^   Bittrex   ^-----------------//
//-------------------------------------------------//





//-------------------------------------------------//
//------------------   Safecex   ------------------//
//-------------------------------------------------//
void Arbitrage::on_SafecexTXGenDepositBTN_clicked()
{
    QString response = GetSafecexTXAddress();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    ui->SafecexTXAddress->setText(ResponseObject.value("deposit").toString());
}
void Arbitrage::on_SafecexBTCGenDepositBTN_clicked()
{
    QString response = GetSafecexBTCAddress();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    ui->SafecexBTCAddress->setText(ResponseObject.value("deposit").toString());
}
void Arbitrage::on_Safecex_Withdraw_MaxTX_Amount_clicked()
{
    QString response = GetSafecexBalance("TX");
    QString str;

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
    double AvailableTX = ResponseObject.value("balance").toDouble();

    ui->SafecexWithdrawTXInput->setText(str.number(AvailableTX,'i',8));
}
void Arbitrage::on_Safecex_Withdraw_MaxBTC_Amount_clicked()
{
    QString response = GetSafecexBalance("BTC");
    QString str;

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj
    double AvailableBTC = ResponseObject.value("balance").toDouble();

    ui->SafecexWithdrawBTCInput->setText(str.number(AvailableBTC,'i',8));
}
void Arbitrage::on_SafecexWithdrawTXBtn_clicked()
{
    double Quantity = ui->SafecexWithdrawTXInput->text().toDouble();
    QString Qstr;
    QString Coin = "TX";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.02),'i',8);
            Msg += " TX to ";
            Msg += ui->SafecexWithdrawTXAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response =  SafecexWithdraw(Quantity, ui->SafecexWithdrawTXAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["message"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
void Arbitrage::on_SafecexWithdrawBTCBtn_clicked()
{
    double Quantity = ui->SafecexWithdrawBTCInput->text().toDouble();
    QString Qstr;
    QString Coin = "BTC";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.0002),'i',8);
            Msg += " BTC to ";
            Msg += ui->SafecexWithdrawBTCAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response =  SafecexWithdraw(Quantity, ui->SafecexWithdrawBTCAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["message"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
QString Arbitrage::BuyTXSafecex(double Quantity, double Rate)
{

    QString str = "";
    QString URL = "https://safecex.com/api/buylimit";
            URL += "?apikey=";
            URL += this->SafecexApiKey;
            URL += "&nonce=";
            URL += GetTimeMicros();
            URL += "&market=TX/BTC&amount=";
            URL += str.number(Quantity,'i',8);
            URL += "&price=";
            URL += str.number(Rate,'i',8);

    QString Response = sendSafecexRequest(URL);
    return Response;
}
void Arbitrage::SafecexBuy_Bittrex(QJsonObject bittrex, QJsonObject safecex)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double safecexQ = safecex.value("asks").toArray().first().toObject().value("amount").toString().toDouble();
    double price = safecex.value("asks").toArray().first().toObject().value("price").toString().toDouble();

    if (bittrexQ > safecexQ) {
        double amount = safecexQ;

        QString Response = BuyTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("status").toString() == "ok") {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Safecex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Safecex", amount, price, false);
        } else {
            QString update = "SafecexBuy - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = bittrexQ;

        QString Response = BuyTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("status").toString() == "ok") {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Safecex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Safecex", amount, price, false);
        } else {
            QString update = "SafecexBuy - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::SellTXSafecex(double Amount, double Price)
{

    QString str = "";
    QString URL = "https://safecex.com/api/selllimit";
            URL += "?apikey=";
            URL += this->SafecexApiKey;
            URL += "&nonce=";
            URL += GetTimeMicros();
            URL += "&market=TX/BTC&amount=";
            URL += str.number(Amount,'i',8);
            URL += "&price=";
            URL += str.number(Price,'i',8);

    QString Response = sendSafecexRequest(URL);
    return Response;
}
void Arbitrage::SafecexSell_Bittrex(QJsonObject bittrex, QJsonObject safecex)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double safecexQ = safecex.value("bids").toArray().first().toObject().value("amount").toString().toDouble();
    double price = safecex.value("bids").toArray().first().toObject().value("price").toString().toDouble();

    if (safecexQ > bittrexQ) {
        double amount = bittrexQ;

        QString Response = SellTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("status").toString() == "ok") {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Safecex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Safecex", amount, price, true);
        } else {
            QString update = "SafecexSell - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = safecexQ;

        QString Response = SellTXSafecex(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("status").toString() == "ok") {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Safecex";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Safecex", amount, price, true);
        } else {
            QString update = "SafecexSell - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::SafecexWithdraw(double Amount, QString Address, QString Coin)
{

    QString str = "";
    QString URL = "https://bittrex.com/api/v1.1/account/withdraw?apikey=";
            URL += this->SafecexApiKey;
            URL += "&currency=";
            URL += Coin;
            URL += "&quantity=";
            URL += str.number(Amount,'i',8);
            URL += "&address=";
            URL += Address;
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendSafecexRequest(URL);
     return Response;
}
QString Arbitrage::GetSafecexOrderBook()
{

    QString  Response = sendSafecexRequest("https://safecex.com/api/getorderbook?market=TX/BTC", false);
    return Response;
}
QString Arbitrage::GetSafecexBalance(QString Currency)
{

    QString URL = "https://safecex.com/api/getbalance?apikey=";
            URL += this->SafecexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&symbol=";
            URL += Currency;

    QString Response = sendSafecexRequest(URL);
     return Response;
}
QString Arbitrage::GetSafecexTXAddress()
{

    QString URL = "https://safecex.com/api/getbalance?apikey=";
            URL += this->SafecexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&symbol=TX";

    QString Response = sendSafecexRequest(URL);
    return Response;
}
QString Arbitrage::GetSafecexBTCAddress()
{

    QString URL = "https://safecex.com/api/getbalance?apikey=";
            URL += this->SafecexApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&symbol=BTC";

    QString Response = sendSafecexRequest(URL);
    return Response;
}
QString Arbitrage::sendSafecexRequest(QString url, bool post)
{

    QString Response = "";
    QString Secret   = this->SafecexSecretKey;

    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req = QNetworkRequest(QUrl(url));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //make this conditional,depending if we are using private api call
    if (post) {
        req.setRawHeader("apisign",HMAC_SHA512_SIGNER(url,Secret).toStdString().c_str()); //set header for bittrex
    }

    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        //success
        Response = reply->readAll();
        //QMessageBox::information(this,"Success",Response);
        delete reply;
    }
    else{
        //failure
        qDebug() << "Failure" <<reply->errorString();
        Response = "Error";
        QMessageBox::information(this,"Error",reply->errorString());
        delete reply;
        }

     return Response;
}
//-------------------------------------------------//
//-----------------^   Safecex   ^-----------------//
//-------------------------------------------------//





//-------------------------------------------------//
//------------------   Extrade   ------------------//
//-------------------------------------------------//
void Arbitrage::on_ExtradeTXGenDepositBTN_clicked()
{
    QString response         =  GetExtradeTXAddress();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
    ui->ExtradeTXAddress->setText(ResponseObject.value("Address").toString());
}
void Arbitrage::on_ExtradeBTCGenDepositBTN_clicked()
{
    QString response         =  GetExtradeBTCAddress();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
    ui->ExtradeBTCAddress->setText(ResponseObject.value("Address").toString());
}
void Arbitrage::on_Extrade_Withdraw_MaxTX_Amount_clicked()
{
    QString response = GetExtradeBalance("TX");
    QString str;

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    double AvailableTX = ResponseObject.value("Available").toDouble();

    ui->ExtradeWithdrawTXInput->setText(str.number(AvailableTX,'i',8));
}
void Arbitrage::on_Extrade_Withdraw_MaxBTC_Amount_clicked()
{
    QString response = GetExtradeBalance("BTC");
    QString str;

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    double AvailableBTC = ResponseObject.value("Available").toDouble();

    ui->ExtradeWithdrawBTCInput->setText(str.number(AvailableBTC,'i',8));
}
void Arbitrage::on_ExtradeWithdrawTXBtn_clicked()
{
    double Quantity = ui->ExtradeWithdrawTXInput->text().toDouble();
    QString Qstr;
    QString Coin = "TX";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.02),'i',8);
            Msg += " TX to ";
            Msg += ui->ExtradeWithdrawTXAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response = ExtradeWithdraw(Quantity, ui->ExtradeWithdrawTXAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["message"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
void Arbitrage::on_ExtradeWithdrawBTCBtn_clicked()
{
    double Quantity = ui->ExtradeWithdrawBTCInput->text().toDouble();
    QString Qstr;
    QString Coin = "BTC";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.0002),'i',8);
            Msg += " BTC to ";
            Msg += ui->ExtradeWithdrawBTCAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response = ExtradeWithdraw(Quantity, ui->ExtradeWithdrawBTCAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["message"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
QString Arbitrage::BuyTXExtrade(QString OrderType, QString OrderSide, double Quantity, double Rate)
{
    QJsonObject params;
    QString str = "";
    QString URL = "https://1ex.trade/api/orders/new";
            URL += "?apikey=";
            URL += this->ExtradeApiKey;
            URL += "&type=";
            URL += OrderType;
            URL += "&side=";
            URL += OrderSide;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&market=TX&currency=BTC&amount=";
            URL += str.number(Quantity,'i',8);
            URL += "&limit_price=";
            URL += str.number(Rate,'i',8);

    QString Response = sendSafecexRequest(URL, true);
    return Response;
}
void Arbitrage::ExtradeBuy_Bittrex(QJsonObject bittrex, QJsonObject extrade)
{
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value(QString("sell")).toObject().value(0).toObject().value(QString("Quantity")).toDouble();
    double extradeQ = extrade.value(QString("order-book")).toObject().value(QString("bid")).toObject().value(0).toObject().value(QString("order_amount")).toDouble();

    if (bittrexQ > extradeQ) {
        QString type = "limit";
        QString side = "buy";
        double amount = extrade.value(QString("order-book")).toObject().value(QString("ask")).toObject().value(0).toObject().value(QString("order_amount")).toDouble();
        double price = extrade.value(QString("order-book")).toObject().value(QString("ask")).toObject().value(0).toObject().value(QString("price")).toDouble();

        QString Response = BuyTXExtrade(type, side, amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+QString::number(amount, 'f', 8)+" Transfercoin for "+QString::number(price, 'f', 8)+" from ExTrade";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Extrade", amount, price, false);
        } else {
                QString update = "ExtradeBuy - Error: "+ResponseObject.value(QString("errors")).toString();
                ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        QString type = "limit";
        QString side = "buy";
        double amount = bittrex.value(QString("result")).toObject().value(QString("sell")).toObject().value(0).toObject().value(QString("Quantity")).toDouble();
        double price = extrade.value(QString("order-book")).toObject().value(QString("bid")).toObject().value(0).toObject().value(QString("price")).toDouble();

        QString Response = BuyTXExtrade(type, side, amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("errors").toBool() == false) {
            QString update = "Purchased "+QString::number(amount, 'f', 8)+" Transfercoin for "+QString::number(price, 'f', 8)+" from ExTrade";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Extrade", amount, price, false);
        } else {
            QString update = "ExtradeBuy - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::SellTXExtrade(QString OrderType, QString OrderSide, double Amount, double Price)
{
    QJsonObject params;
    QString str = "";
    QString URL = "https://1ex.trade/api/orders/new";
            URL += "?apikey=";
            URL += this->ExtradeApiKey;
            URL += "&type=";
            URL += OrderType;
            URL += "&side=";
            URL += OrderSide;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&market=TX&currency=BTC&amount=";
            URL += str.number(Amount,'i',8);
            URL += "&limit_price=";
            URL += str.number(Price,'i',8);

    QString Response = sendExtradeRequest(URL, true);
    return Response;
}
QString Arbitrage::ExtradeWithdraw(double Amount, QString Address, QString Coin)
{
    QJsonObject params;
    QString str = "";
    QString URL = "https://1ex.trade/api/withdrawals/new?apikey=";
            URL += this->ExtradeApiKey;
            URL += "&currency=";
            URL += Coin;
            URL += "&amount=";
            URL += str.number(Amount,'i',8);
            URL += "&address=";
            URL += Address;
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendExtradeRequest(URL, true);
     return Response;
}
QString Arbitrage::GetExtradeOrderBook()
{
    QString Response = sendExtradeRequest("https://1ex.trade/api/order-book?market=TX&currency=BTC", false);
    return Response;
}
QString Arbitrage::GetExtradeBalance(QString Currency)
{
    QString URL = "https://1ex.trade/api/balances-and-info?apikey=";
            URL += this->ExtradeApiKey;
            URL += "&nonce=";
            URL += GetTimeMicros();
            URL += "&currency=";
            URL += Currency;

    QString Response = sendExtradeRequest(URL, true);
     return Response;
}
QString Arbitrage::GetExtradeTXAddress()
{
    QString URL = "https://1ex.trade/api/crypto-deposit-address/get?apikey=";
            URL += this->ExtradeApiKey;
            URL += "&nonce="
            URL += GetTimeMicros();
            URL += "&market=TX";

    QString Response = sendExtradeRequest(URL, true);
    return Response;
}
QString Arbitrage::GetExtradeBTCAddress()
{
    QString URL = "https://1ex.trade/api/crypto-deposit-address/get?apikey=";
            URL += this->ExtradeApiKey;
            URL += "&nonce=";
            URL += GetTimeMicros();
            URL += "&market=BTC";

    QString Response = sendExtradeRequest(URL, true);
    return Response;
}
QString Arbitrage::sendExtradeRequest(QString url, bool post)
{

    QString Response = "";
    QString Secret   = this->ExtradeSecretKey;
    if (post)
    {
        url += "&signature=";
        url += HMAC_SHA256_SIGNER(url,Secret).toStdString().c_str();
    }

    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req = QNetworkRequest(QUrl(url));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //data.append(QUrl::encodedQuery(strJson))

    QSslConfiguration config = QSslConfiguration::defaultConfiguration();

    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        //success
        Response = reply->readAll();
        QMessageBox::information(this,"success",Response);
        delete reply;
    }
    else{
        //failure
        qDebug() << "Failure" <<reply->errorString();
        Response = "Error";
        QMessageBox::information(this,"Error",reply->errorString());
        delete reply;
        }

     return Response;
}
//-------------------------------------------------//
//-----------------^   Extrade   ^-----------------//
//-------------------------------------------------//





//-------------------------------------------------//
//-------------------   Yobit   -------------------//
//-------------------------------------------------//
void Arbitrage::on_YobitTXGenDepositBTN_clicked()
{
    QString response = GetYobitTXAddress();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    ui->YobitTXAddress->setText(ResponseObject.value("return").toObject().value("address").toString());
}
void Arbitrage::on_YobitBTCGenDepositBTN_clicked()
{
    QString response = GetYobitBTCAddress();
    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

    ui->YobitBTCAddress->setText(ResponseObject.value("return").toObject().value("address").toString());
}
void Arbitrage::on_Yobit_Withdraw_MaxTX_Amount_clicked()
{
    QString response = GetYobitBalance();
    QString str;

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
    double AvailableTX = ResponseObject.value("return").toObject().value("funds").toObject().value("tx").toDouble();

    ui->YobitWithdrawTXInput->setText(str.number(AvailableTX,'i',8));
}
void Arbitrage::on_Yobit_Withdraw_MaxBTC_Amount_clicked()
{
    QString response = GetYobitBalance();
    QString str;

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj
    double AvailableBTC = ResponseObject.value("return").toObject().value("funds").toObject().value("btc").toDouble();

    ui->YobitWithdrawBTCInput->setText(str.number(AvailableBTC,'i',8));
}
void Arbitrage::on_YobitWithdrawTXBtn_clicked()
{
    double Quantity = ui->YobitWithdrawTXInput->text().toDouble();
    QString Qstr;
    QString Coin = "TX";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.02),'i',8);
            Msg += " TX to ";
            Msg += ui->YobitWithdrawTXAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response =  YobitWithdraw(Quantity, ui->YobitWithdrawTXAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["errors"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
void Arbitrage::on_YobitWithdrawBTCBtn_clicked()
{
    double Quantity = ui->YobitWithdrawBTCInput->text().toDouble();
    QString Qstr;
    QString Coin = "BTC";
    QString Msg = "Are you sure you want to Withdraw ";
            Msg += Qstr.number((Quantity - 0.0002),'i',8);
            Msg += " BTC to ";
            Msg += ui->YobitWithdrawBTCAddress->text();
            Msg += " ?";

    QMessageBox::StandardButton reply;
    reply = QMessageBox::question(this,"Withdraw",Msg,QMessageBox::Yes|QMessageBox::No);

    if(reply != QMessageBox::Yes)
    {
        return;
    }

    WalletModel::UnlockContext ctx(model->requestUnlock());
    if(!ctx.isValid())
    {
        // Unlock wallet was cancelled
        return;
    }

        QString Response =  YobitWithdraw(Quantity, ui->YobitWithdrawBTCAddress->text(), Coin);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject["success"].toBool() == false){
            QMessageBox::information(this,"Failed",ResponseObject["errors"].toString());

        }else if (ResponseObject["success"].toBool() == true){
            QMessageBox::information(this,"Success","Withdrawal Successful !");
        }
}
QString Arbitrage::BuyTXYobit(double Quantity, double Rate)
{

    QString str = "";
    QString URL = "https://yobit.net/tapi?method=Trade";
            URL += "&nonce=";
            URL += GetTimeMicros();
            URL += "&pair=tx_btc&type=buy&amount=";
            URL += str.number(Quantity,'i',8);
            URL += "&rate=";
            URL += str.number(Rate,'i',8);

    QString Response = sendSafecexRequest(URL);
    return Response;
}
void Arbitrage::YobitBuy_Bittrex(QJsonObject bittrex, QJsonObject yobit)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double yobitQ = yobit.value("asks").toArray().first().toArray().last().toDouble();
    double price = yobit.value("asks").toArray().first().toArray().first().toDouble();

    if (bittrexQ > yobitQ) {
        double amount = yobitQ;

        QString Response = BuyTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, false);
        } else {
            QString update = "YobitBuy_Bittrex - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = bittrexQ;

        QString Response = BuyTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, false);
        } else {
            QString update = "YobitBuy_Bittrex - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
void Arbitrage::YobitBuy_Safecex(QJsonObject safecex, QJsonObject yobit)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double safecexQ = safecex.value("asks").toArray().first().toObject().value("amount").toString().toDouble();
    double yobitQ = yobit.value("asks").toArray().first().toArray().last().toDouble();
    double price = yobit.value("asks").toArray().first().toArray().first().toDouble();

    if (safecexQ > yobitQ) {
        double amount = yobitQ;

        QString Response = BuyTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, false);
        } else {
            QString update = "YobitBuy_Safecex - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = safecexQ;

        QString Response = BuyTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Purchased "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" from Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, false);
        } else {
            QString update = "YobitBuy_Safecex - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::SellTXYobit(double Amount, double Price)
{

    QString str = "";
    QString URL = "https://yobit.net/tapi?method=Trade";
            URL += "&pair=tx_btc&type=buyamount=";
            URL += str.number(Amount,'i',8);
            URL += "&rate=";
            URL += str.number(Price,'i',8);
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendSafecexRequest(URL);
    return Response;
}
void Arbitrage::YobitSell_Bittrex(QJsonObject bittrex, QJsonObject yobit)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double bittrexQ = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Quantity").toDouble();
    double yobitQ = yobit.value("bids").toArray().first().toArray().last().toDouble();
    double price = yobit.value("bids").toArray().first().toArray().first().toDouble();

    if (yobitQ > bittrexQ) {
        double amount = bittrexQ;

        QString Response = SellTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, true);
        } else {
            QString update = "YobitSell_Bittrex - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = yobitQ;

        QString Response = SellTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, true);
        } else {
            QString update = "YobitSell_Bittrex - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
void Arbitrage::YobitSell_Safecex(QJsonObject safecex, QJsonObject yobit)
{
    QString str = "";
    int RowCount = 0;
    RowCount = ui->DebugHistory->rowCount();
    ui->DebugHistory->insertRow(RowCount);
    double safecexQ = safecex.value("asks").toArray().first().toObject().value("amount").toString().toDouble();
    double yobitQ = yobit.value("bids").toArray().first().toArray().last().toDouble();
    double price = yobit.value("bids").toArray().first().toArray().first().toDouble();

    if (yobitQ > safecexQ) {
        double amount = safecexQ;

        QString Response = SellTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, true);
        } else {
            QString update = "YobitSell_Safecex - Error: "+ResponseObject.value(QString("errors")).toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        double amount = safecexQ;

        QString Response = SellTXYobit(amount, price);
        QJsonDocument jsonResponse = QJsonDocument::fromJson(Response.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj
        if (ResponseObject.value("success").toBool() == true) {
            QString update = "Sold "+str.number(amount, 'i', 8)+" Transfercoin for "+str.number(price, 'i', 8)+" on Yobit";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            UpdateArbTable("Yobit", amount, price, true);
        } else {
            QString update = "YobitSell_Safecex - Error: "+ResponseObject.value("errors").toString();
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    }
}
QString Arbitrage::YobitWithdraw(double Amount, QString Address, QString Coin)
{

    QString str = "";
    QString URL = "https://yobit.net/tapi?method=WithdrawCoinsToAddress";
            URL += "&coinName=";
            URL += Coin;
            URL += "&amount=";
            URL += str.number(Amount,'i',8);
            URL += "&address=";
            URL += Address;
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendYobitRequest(URL);
     return Response;
}
QString Arbitrage::GetYobitOrderBook()
{

    QString  Response = sendYobitRequest("https://yobit.net/api/3/depth/tx_btc", false);
    return Response;
}
QString Arbitrage::GetYobitBalance()
{
    QString URL = "https://yobit.net/tapi?method=getInfo";
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendYobitRequest(URL);
     return Response;
}
QString Arbitrage::GetYobitTXAddress()
{
    QString URL = "https://yobit.net/tapi?method=GetDepositAddress";
            URL += "&coinName=tx";
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendYobitRequest(URL);
    return Response;
}
QString Arbitrage::GetYobitBTCAddress()
{
    QString URL = "https://yobit.net/tapi?method=GetDepositAddress";
            URL += "&coinName=btc";
            URL += "&nonce=";
            URL += GetTimeMicros();

    QString Response = sendYobitRequest(URL);
    return Response;
}
QString Arbitrage::sendYobitRequest(QString url, bool post)
{

    QString Response = "";
    QString Secret   = this->YobitSecretKey;

    // create custom temporary event loop on stack
    QEventLoop eventLoop;

    // "quit()" the event-loop, when the network request "finished()"
    QNetworkAccessManager mgr;
    QObject::connect(&mgr, SIGNAL(finished(QNetworkReply*)), &eventLoop, SLOT(quit()));

    // the HTTP request
    QNetworkRequest req = QNetworkRequest(QUrl(url));

    req.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    //make this conditional,depending if we are using private api call
    if (post) {
        req.setRawHeader("Sign",HMAC_SHA512_SIGNER_YOBIT(url,Secret).toStdString().c_str()); //set header for yobit
        req.setRawHeader("Key",this->YobitApiKey.toStdString().c_str()); //set header for yobit
    }

    QNetworkReply *reply = mgr.get(req);
    eventLoop.exec(); // blocks stack until "finished()" has been called

    if (reply->error() == QNetworkReply::NoError) {
        //success
        Response = reply->readAll();
        QMessageBox::information(this,"Success",Response);
        delete reply;
    }
    else{
        //failure
        qDebug() << "Failure" <<reply->errorString();
        Response = "Error";
        QMessageBox::information(this,"Error",reply->errorString());
        delete reply;
        }

     return Response;
}
//-------------------------------------------------//
//------------------^   Yobit   ^------------------//
//-------------------------------------------------//





//-------------------------------------------------//
//---------------   Morph Objects   ---------------//
//-------------------------------------------------//
QString Arbitrage::BittrexTimeStampToReadable(QString DateTime)
{
    //Seperate Time and date.
    int TPos = DateTime.indexOf("T");
    int sPos = DateTime.indexOf(".");
    QDateTime Date = QDateTime::fromString(DateTime.left(TPos),"yyyy-MM-dd"); //format to convert from
    DateTime.remove(sPos,sizeof(DateTime));
    DateTime.remove(0,TPos+1);
    QDateTime Time = QDateTime::fromString(DateTime.right(TPos),"hh:mm:ss");

    //Reconstruct time and date in our own format, one that QDateTime will recognise.
    QString DisplayDate = Date.toString("dd/MM/yyyy") + " " + Time.toString("hh:mm:ss A"); //formats to convert to

    return DisplayDate;
}
QString Arbitrage::HMAC_SHA512_SIGNER(QString UrlToSign, QString Secret)
{

    QString retval = "";

    QByteArray byteArray = UrlToSign.toUtf8();
    const char* URL = byteArray.constData();

    QByteArray byteArrayB = Secret.toUtf8();
    const char* Secretkey = byteArrayB.constData();

    const EVP_MD *md = EVP_sha512();
    unsigned char* digest = NULL;

    // Using sha512 hash engine here.
    digest = HMAC(md,  Secretkey, strlen( Secretkey), (unsigned char*) URL, strlen( URL), NULL, NULL);

    // Be careful of the length of string with the choosen hash engine. SHA512 produces a 64-byte hash value which rendered as 128 characters.
    // Change the length accordingly with your choosen hash engine
    char mdString[129] = { 0 };

    for(int i = 0; i < 64; i++){
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    retval = mdString;
    //qDebug() << "HMAC digest:"<< retval;

    return retval;
}
// Returns the relative Request-URI from url. For example, given the url
// "http://www.google.com/search?q=xyz", it will return "/search?q=xyz".
//
// url need not be prefixed with a protocol; i.e. "google.com" is valid.
//
// If url contains a protocol (i.e. "http://"), the Request-URI begins with the
// first "/" after the protocol. Otherwise, the Request-URI begins with the
// first "/".
//
// If url does not contain a Request-URI, its Request-URI is "/", the server
// root.
string Arbitrage::ParseRequestUri(std::string url) {
  const std::string protocol_identifier("://");

  std::size_t pos = url.find(protocol_identifier);

  if (pos != std::string::npos)
    pos = url.find_first_of("?", pos + protocol_identifier.length());
  else
    pos = url.find_first_of("?");

  if (pos != std::string::npos)
    return url.substr(pos);

  return "/";
}
QString Arbitrage::HMAC_SHA512_SIGNER_YOBIT(QString UrlToSign, QString Secret)
{

    QString retval = "";
    string test = ParseRequestUri(UrlToSign.toStdString());
    UrlToSign = QString::fromStdString(test);

    QByteArray byteArray = UrlToSign.toUtf8();
    const char* URL = byteArray.constData();

    QByteArray byteArrayB = Secret.toUtf8();
    const char* Secretkey = byteArrayB.constData();

    const EVP_MD *md = EVP_sha512();
    unsigned char* digest = NULL;

    // Using sha512 hash engine here.
    digest = HMAC(md,  Secretkey, strlen( Secretkey), (unsigned char*) URL, strlen( URL), NULL, NULL);

    // Be careful of the length of string with the choosen hash engine. SHA512 produces a 64-byte hash value which rendered as 128 characters.
    // Change the length accordingly with your choosen hash engine
    char mdString[129] = { 0 };

    for(int i = 0; i < 64; i++){
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    retval = mdString;
    //qDebug() << "HMAC digest:"<< retval;

    return retval;
}
QString Arbitrage::HMAC_SHA256_SIGNER(QString UrlToSign, QString Secret)
{

    QString retval = "";
    string test = ParseRequestUri(UrlToSign.toStdString());
    UrlToSign = QString::fromStdString(test);

    QByteArray byteArray = UrlToSign.toUtf8();
    const char* URL = byteArray.toBase64();

    QByteArray byteArrayB = Secret.toUtf8();
    const char* Secretkey = byteArrayB.constData();

    const EVP_MD *md = EVP_sha256();
    unsigned char* digest = NULL;

    // Using sha512 hash engine here.
    digest = HMAC(md,  Secretkey, strlen( Secretkey), (unsigned char*) URL, strlen( URL), NULL, NULL);

    // Be careful of the length of string with the choosen hash engine. SHA256 produces a 32-byte hash value which rendered as 64 characters.
    // Change the length accordingly with your choosen hash engine
    char mdString[65] = { 0 };

    for(int i = 0; i < 32; i++){
        sprintf(&mdString[i*2], "%02x", (unsigned int)digest[i]);
    }
    retval = mdString;
    //QMessageBox::information(this,"HMAC Digest","HMAC_SHA256_SIGNER: " + retval);

    return retval;
}
QJsonObject Arbitrage::GetResultObjectFromJSONObject(QString response)
{

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());          //get json from str.
    QJsonObject  ResponseObject = jsonResponse.object();                              //get json obj
    QJsonObject  ResultObject   = ResponseObject.value(QString("result")).toObject(); //get result object

    return ResultObject;
}
QJsonObject Arbitrage::GetResultObjectFromJSONArray(QString response)
{

    QJsonDocument jsonResponsea = QJsonDocument::fromJson(response.toUtf8());
    QJsonObject   jsonObjecta   = jsonResponsea.object();
    QJsonArray    jsonArraya    = jsonObjecta["result"].toArray();
    QJsonObject   obj;

    foreach (const QJsonValue & value, jsonArraya)
    {
        obj = value.toObject();
    }
    return obj;
}
QJsonArray Arbitrage::GetResultArrayFromJSONObject(QString response)
{

    QJsonDocument jsonResponse = QJsonDocument::fromJson(response.toUtf8());
    QJsonObject   jsonObject   = jsonResponse.object();
    QJsonArray    jsonArray    = jsonObject["result"].toArray();

    return jsonArray;
}
//-------------------------------------------------//
//--------------^   Morph Objects   ^--------------//
//-------------------------------------------------//





//-------------------------------------------------//
//-----------------   Arbitrage   -----------------//
//-------------------------------------------------//
void Arbitrage::StartArbitrage()
{
    ui->StopArbButton->setEnabled(true);
    QString BittrexOrders;
    QJsonObject BittrexObject;
    QString SafecexOrders;
    QJsonObject SafecexObject;
    QString ExtradeOrders;
    QJsonObject ExtradeObject;
    QString YobitOrders;
    QJsonObject YobitObject;
    if (this->EnableBittrex == true) {
        BittrexOrders = GetBittrexOrderBook();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(BittrexOrders.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        BittrexObject = ResponseObject;
    }
    if (this->EnableSafecex == true) {
        SafecexOrders = GetSafecexOrderBook();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(SafecexOrders.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        SafecexObject = ResponseObject;
    }
    if (this->EnableExtrade == true) {
        ExtradeOrders = GetExtradeOrderBook();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(ExtradeOrders.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        ExtradeObject = ResponseObject;
    }
    if (this->EnableYobit == true) {
        YobitOrders = GetYobitOrderBook();
        QJsonDocument jsonResponse = QJsonDocument::fromJson(YobitOrders.toUtf8());          //get json from str.
        QJsonObject ResponseObject = jsonResponse.object();                              //get json obj

        YobitObject = ResponseObject;
    }
    if (this->EnableBittrex == true && this->EnableExtrade == true) {

        BuyExtradeSellBittrex(ExtradeObject, BittrexObject);
        BuyBittrexSellExtrade(ExtradeObject, BittrexObject);
    }
    if (this->EnableBittrex == true && this->EnableSafecex == true) {

        BuySafecexSellBittrex(SafecexObject, BittrexObject);
        BuyBittrexSellSafecex(SafecexObject, BittrexObject);
    }
    if (this->EnableBittrex == true && this->EnableYobit == true) {
        BuyYobitSellBittrex(YobitObject, BittrexObject);
        BuyBittrexSellYobit(YobitObject, BittrexObject);
    }
    if (this->EnableSafecex == true && this->EnableExtrade == true) {
        //BuyExtradeSellSafeCex(SafecexObject, ExtradeObject);
        //BuySafeCexSellExtrade(SafecexObject, ExtradeObject);
    }
    if (this->EnableSafecex == true && this->EnableYobit == true) {
        BuyYobitSellSafecex(SafecexObject, YobitObject);
        BuySafecexSellYobit(SafecexObject, YobitObject);
    }
    if (this->EnableExtrade == true && this->EnableYobit == true) {
        //BuyExtradeSellYobit(ExtradeObject, YobitObject);
        //BuyYobitSellExtrade(ExtradeObject, YobitObject);
    }
}
void Arbitrage::UpdateArbTable(QString exchange, double amount, double price, bool sell)
{
    int RowCount = ui->ArbHistoryTable->rowCount();
    ui->ArbHistoryTable->insertRow(RowCount);
    time_t now = time(0);
    char* dt = ctime(&now);
    QString str = "";
    ui->ArbHistoryTable->setItem(RowCount, 0, new QTableWidgetItem(dt));
    ui->ArbHistoryTable->setItem(RowCount, 1, new QTableWidgetItem(exchange));
    ui->ArbHistoryTable->setItem(RowCount, 2, new QTableWidgetItem(sell ? "sell" : "buy"));
    ui->ArbHistoryTable->setItem(RowCount, 3, new QTableWidgetItem(str.number(amount,'i',8)));
    ui->ArbHistoryTable->setItem(RowCount, 4, new QTableWidgetItem(str.number(price,'i',8)));
    ui->ArbHistoryTable->setItem(RowCount, 5, new QTableWidgetItem(str.number((price * amount),'i',8)));
    if (sell){
        double sellPrice = (price * amount);
        double buyPrice = ui->ArbHistoryTable->item((RowCount-1), 5)->text().toDouble();
        double Profit = sellPrice - buyPrice;
        this->TotalEarned += Profit;
        ui->ArbHistoryTable->setItem(RowCount, 6, new QTableWidgetItem(Profit));
    }
}
void Arbitrage::on_StartArbButton_clicked()
{
    StartArbitrage();
    if(this->arbtimerid == 0)
    {
        //Timer is not set,lets create one.
        this->arbtimer = new QTimer(this);
        connect(arbtimer, SIGNAL(timeout()), this, SLOT(StartArbitrage()));
        this->arbtimer->start((ui->Interval->text().toDouble() * 1000));
        this->arbtimerid = this->arbtimer->timerId();
    } else {
        this->arbtimer->start((ui->Interval->text().toDouble() * 1000));
    }
    ui->StartArbButton->setEnabled(false);
    ui->StopArbButton->setEnabled(false);
}
void Arbitrage::on_StopArbButton_clicked()
{
    this->arbtimer->stop();
    ui->StartArbButton->setEnabled(true);
}

// Bittrex
void Arbitrage::BuyExtradeSellBittrex(QJsonObject extrade, QJsonObject bittrex) {
    int RowCount = 0;
    double percentGain = ui->PercentGain->text().toDouble();
    QString str = "";

    double BittrexPrice = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();
    double ExtradePrice = extrade.value(QString("order-book")).toObject().value("ask").toArray().first().toObject().value("price").toString().toDouble();

    if (ExtradePrice < BittrexPrice){
        if ((((BittrexPrice / ExtradePrice) * 100) - 100) > percentGain) {
            //ExtradeBuy(bittrex, extrade);
            //BittrexSell(bittrex, extrade);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / ExtradePrice) * 100) - 100),'i',8);
                    update += "% gain for buying on ExTrade(";
                    update += str.number(ExtradePrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            return;
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / ExtradePrice) * 100) - 100),'i',8);
                    update += "% gain for buying on ExTrade(";
                    update += str.number(ExtradePrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Waiting until higher than ";
                    update += str.number(percentGain,'i',4);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
            return;
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on ExTrade(";
                update += str.number(ExtradePrice,'i',8);
                update += ") and selling on Bittrex(";
                update += str.number(BittrexPrice,'i',8);
                update += ")";
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        return;
    }
}
void Arbitrage::BuyBittrexSellExtrade(QJsonObject extrade, QJsonObject bittrex) {
    int RowCount = 0;
    int percentGain = ui->PercentGain->text().toDouble();
    QString str = "";
    double BittrexPrice = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();
    double ExtradePrice = extrade.value(QString("order-book")).toObject().value("bid").toArray().first().toObject().value("price").toString().toDouble();

    if (BittrexPrice < ExtradePrice){
        if ( (((ExtradePrice / BittrexPrice) * 100) - 100) > percentGain) {
            //BittrexBuy(bittrex, extrade);
            //ExtradeSell(bittrex, extrade);
            QString update = "Arb opportunity. ";
                    update += str.number((((ExtradePrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") and selling on Extrade(";
                    update += str.number(ExtradePrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((ExtradePrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += BittrexPrice;
                    update += ") and selling on Extrade(";
                    update += ExtradePrice;
                    update +=") Waiting until higher than ";
                    update += percentGain;
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Bittrex(";
                update += QString::number(BittrexPrice, 'f', 8);
                update += ") and selling on Extrade(";
                update += QString::number(ExtradePrice, 'f', 8)+')';
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuySafecexSellBittrex(QJsonObject safecex, QJsonObject bittrex) {
    int RowCount = 0;
    double percentGain = ui->PercentGain->text().toDouble();
    QString str = "";

    double BittrexPrice = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();
    double SafecexPrice = safecex.value("asks").toArray().first().toObject().value("price").toString().toDouble();

    /*if (safecex.value("asks").toArray().first().toObject().value("price").isArray() == true) {
        QMessageBox::information(this,"safecextest1","ARRAY");
    } else if (safecex.value("asks").toArray().first().toObject().value("price").isString()) {
        QMessageBox::information(this,"safecextest2",safecex.value("asks").toArray().first().toObject().value("price").isString());
    } else if (safecex.value("asks").toArray().first().toObject().value("price").isObject()) {
        QMessageBox::information(this,"safecextest3","OBJECT");
    } else if (safecex.value("asks").toArray().first().toObject().value("price").isUndefined()) {
        QMessageBox::information(this,"safecextest4","UNDEFINED");
    }*/

    //QMessageBox::information(this,"bittrextest5",bittrex.value("buy").toObject().value(0).toString());
    //QMessageBox::information(this,"bittrextest6",bittrex.value("buy").toObject().value(0).toObject().value("Rate").toString());
    //QMessageBox::information(this,"safecextest1",safecex.toString());
    //QMessageBox::information(this,"safecextest1",safecex.value("asks").toString());
    //QMessageBox::information(this,"safecextest2",safecex.value("asks").toObject().value(0).toString());
    //QMessageBox::information(this,"safecextest3",safecex.value("asks").toObject().value(0).toObject().value("price").toString());

    if (SafecexPrice < BittrexPrice){
        if ((((BittrexPrice / SafecexPrice) * 100) - 100) > percentGain) {
            SafecexBuy_Bittrex(bittrex, safecex);
            BittrexSell_Safecex(bittrex, safecex);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / SafecexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem((RowCount), 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / SafecexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Waiting until higher than ";
                    update += str.number(percentGain,'i',4);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Safecex(";
                update += str.number(SafecexPrice,'i',8);
                update += ") and selling on Bittrex(";
                update += str.number(BittrexPrice,'i',8);
                update += ")";
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuyBittrexSellSafecex(QJsonObject safecex, QJsonObject bittrex) {
    int RowCount = 0;
    int percentGain = ui->PercentGain->text().toDouble();
    QString str = "";
    double BittrexPrice = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();
    double SafecexPrice = safecex.value("bids").toArray().first().toObject().value("price").toString().toDouble();

    if (BittrexPrice < SafecexPrice){
        if ( (((SafecexPrice / BittrexPrice) * 100) - 100) > percentGain) {
            BittrexBuy_Safecex(bittrex, safecex);
            SafecexSell_Bittrex(bittrex, safecex);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") and selling on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice, 'i', 8);
                    update += ") and selling on Safecex(";
                    update += str.number(SafecexPrice, 'i', 8);
                    update +=") Waiting until higher than ";
                    update += str.number(percentGain, 'i', 8);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Bittrex(";
                update += str.number(BittrexPrice, 'i', 8);
                update += ") and selling on Safecex(";
                update += str.number(SafecexPrice, 'i', 8)+')';
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuyYobitSellBittrex(QJsonObject yobit, QJsonObject bittrex) {
    int RowCount = 0;
    double percentGain = ui->PercentGain->text().toDouble();
    QString str = "";

    double BittrexPrice = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();
    double YobitPrice = yobit.value("asks").toArray().first().toArray().first().toDouble();

    if (YobitPrice < BittrexPrice){
        if ((((BittrexPrice / YobitPrice) * 100) - 100) > percentGain) {
            YobitBuy_Bittrex(bittrex, yobit);
            BittrexSell_Yobit(bittrex, yobit);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / YobitPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem((RowCount), 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / YobitPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Waiting until higher than ";
                    update += str.number(percentGain,'i',4);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Yobit(";
                update += str.number(YobitPrice,'i',8);
                update += ") and selling on Bittrex(";
                update += str.number(BittrexPrice,'i',8);
                update += ")";
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuyBittrexSellYobit(QJsonObject yobit, QJsonObject bittrex) {
    int RowCount = 0;
    int percentGain = ui->PercentGain->text().toDouble();
    QString str = "";
    double BittrexPrice = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();
    double YobitPrice = yobit.value("bids").toArray().first().toArray().first().toDouble();

    if (BittrexPrice < YobitPrice){
        if ( (((YobitPrice / BittrexPrice) * 100) - 100) > percentGain) {
            BittrexBuy_Yobit(bittrex, yobit);
            YobitSell_Bittrex(bittrex, yobit);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((YobitPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") and selling on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((YobitPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice, 'i', 8);
                    update += ") and selling on Yobit(";
                    update += str.number(YobitPrice, 'i', 8);
                    update +=") Waiting until higher than ";
                    update += str.number(percentGain, 'i', 8);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Bittrex(";
                update += str.number(BittrexPrice, 'i', 8);
                update += ") and selling on Yobit(";
                update += str.number(YobitPrice, 'i', 8)+')';
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
// Bittrex



// Safecex
void Arbitrage::BuyYobitSellSafecex(QJsonObject yobit, QJsonObject safecex) {
    int RowCount = 0;
    double percentGain = ui->PercentGain->text().toDouble();
    QString str = "";

    double SafecexPrice = safecex.value("bids").toArray().first().toObject().value("price").toString().toDouble();
    double YobitPrice = yobit.value("asks").toArray().first().toArray().first().toDouble();

    if (YobitPrice < SafecexPrice){
        if ((((SafecexPrice / YobitPrice) * 100) - 100) > percentGain) {
            YobitBuy_Safecex(safecex, yobit);
            SafecexSell_Yobit(safecex, yobit);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / YobitPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") and selling on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem((RowCount), 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / YobitPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") and selling on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") Waiting until higher than ";
                    update += str.number(percentGain,'i',4);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Yobit(";
                update += str.number(YobitPrice,'i',8);
                update += ") and selling on Safecex(";
                update += str.number(SafecexPrice,'i',8);
                update += ")";
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuySafecexSellYobit(QJsonObject yobit, QJsonObject safecex) {
    int RowCount = 0;
    int percentGain = ui->PercentGain->text().toDouble();
    QString str = "";
    double SafecexPrice = safecex.value("asks").toArray().first().toObject().value("price").toString().toDouble();
    double YobitPrice = yobit.value("bids").toArray().first().toArray().first().toDouble();

    if (SafecexPrice < YobitPrice){
        if ( (((YobitPrice / SafecexPrice) * 100) - 100) > percentGain) {
            SafecexBuy_Yobit(safecex, yobit);
            YobitSell_Safecex(safecex, yobit);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((YobitPrice / SafecexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") and selling on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Safecex(";
                    update += str.number(SafecexPrice, 'i', 8);
                    update += ") and selling on Yobit(";
                    update += str.number(YobitPrice, 'i', 8);
                    update +=") Waiting until higher than ";
                    update += str.number(percentGain, 'i', 8);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Safecex(";
                update += str.number(SafecexPrice, 'i', 8);
                update += ") and selling on Yobit(";
                update += str.number(YobitPrice, 'i', 8)+')';
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
/* UNFINISHED
void Arbitrage::BuySafecexSellExtrade(QJsonObject extrade, QJsonObject safecex) {
    int RowCount = 0;
    double percentGain = ui->PercentGain->text().toDouble();
    QString str = "";

    double BittrexPrice = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();
    double SafecexPrice = safecex.value("asks").toArray().first().toObject().value("price").toString().toDouble();

    if (SafecexPrice < BittrexPrice){
        if ((((BittrexPrice / SafecexPrice) * 100) - 100) > percentGain) {
            SafecexBuy_Bittrex(bittrex, safecex);
            BittrexSell_Safecex(bittrex, safecex);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / SafecexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem((RowCount), 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / SafecexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Waiting until higher than ";
                    update += str.number(percentGain,'i',4);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Safecex(";
                update += str.number(SafecexPrice,'i',8);
                update += ") and selling on Bittrex(";
                update += str.number(BittrexPrice,'i',8);
                update += ")";
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuyExtradeSellSafecex(QJsonObject extrade, QJsonObject safecex) {
    int RowCount = 0;
    int percentGain = ui->PercentGain->text().toDouble();
    QString str = "";
    double BittrexPrice = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();
    double SafecexPrice = safecex.value("bids").toArray().first().toObject().value("price").toString().toDouble();

    if (BittrexPrice < SafecexPrice){
        if ( (((SafecexPrice / BittrexPrice) * 100) - 100) > percentGain) {
            BittrexBuy_Safecex(bittrex, safecex);
            SafecexSell_Bittrex(bittrex, safecex);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") and selling on Safecex(";
                    update += str.number(SafecexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice, 'i', 8);
                    update += ") and selling on Safecex(";
                    update += str.number(SafecexPrice, 'i', 8);
                    update +=") Waiting until higher than ";
                    update += str.number(percentGain, 'i', 8);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Bittrex(";
                update += str.number(BittrexPrice, 'i', 8);
                update += ") and selling on Safecex(";
                update += str.number(SafecexPrice, 'i', 8)+')';
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
// Safecex

// Extrade
void Arbitrage::BuyYobitSellExtrade(QJsonObject yobit, QJsonObject extrade) {
    int RowCount = 0;
    double percentGain = ui->PercentGain->text().toDouble();
    QString str = "";

    double BittrexPrice = bittrex.value(QString("result")).toObject().value("buy").toArray().first().toObject().value("Rate").toDouble();
    double YobitPrice = yobit.value("asks").toArray().first().toObject().value("price").toString().toDouble();

    if (YobitPrice < BittrexPrice){
        if ((((BittrexPrice / YobitPrice) * 100) - 100) > percentGain) {
            YobitBuy_Bittrex(bittrex, yobit);
            BittrexSell_Yobit(bittrex, yobit);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / YobitPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem((RowCount), 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((BittrexPrice / YobitPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") and selling on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") Waiting until higher than ";
                    update += str.number(percentGain,'i',4);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Yobit(";
                update += str.number(YobitPrice,'i',8);
                update += ") and selling on Bittrex(";
                update += str.number(BittrexPrice,'i',8);
                update += ")";
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
void Arbitrage::BuyExtradeSellYobit(QJsonObject yobit, QJsonObject extrade) {
    int RowCount = 0;
    int percentGain = ui->PercentGain->text().toDouble();
    QString str = "";
    double BittrexPrice = bittrex.value(QString("result")).toObject().value("sell").toArray().first().toObject().value("Rate").toDouble();
    double YobitPrice = yobit.value("bids").toArray().first().toObject().value("price").toString().toDouble();

    if (BittrexPrice < YobitPrice){
        if ( (((YobitPrice / BittrexPrice) * 100) - 100) > percentGain) {
            BittrexBuy_Safecex(bittrex, yobit);
            SafecexSell_Bittrex(bittrex, yobit);
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((YobitPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice,'i',8);
                    update += ") and selling on Yobit(";
                    update += str.number(YobitPrice,'i',8);
                    update += ") Placing orders.";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        } else {
            RowCount = ui->DebugHistory->rowCount();
            ui->DebugHistory->insertRow(RowCount);
            QString update = "Arb opportunity. ";
                    update += str.number((((SafecexPrice / BittrexPrice) * 100) - 100),'i',8);
                    update += "% gain for buying on Bittrex(";
                    update += str.number(BittrexPrice, 'i', 8);
                    update += ") and selling on Yobit(";
                    update += str.number(YobitPrice, 'i', 8);
                    update +=") Waiting until higher than ";
                    update += str.number(percentGain, 'i', 8);
                    update += "% Gain";
            ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
        }
    } else {
        RowCount = ui->DebugHistory->rowCount();
        ui->DebugHistory->insertRow(RowCount);
        QString update = "No arb opportunities for buying on Bittrex(";
                update += str.number(BittrexPrice, 'i', 8);
                update += ") and selling on Yobit(";
                update += str.number(YobitPrice, 'i', 8)+')';
        ui->DebugHistory->setItem(RowCount, 0, new QTableWidgetItem(update.toUtf8().constData()));
    }
}
// Extrade




// Extrade
//-------------------------------------------------//
//----------------^   Arbitrage   ^----------------//
//-------------------------------------------------//





void Arbitrage::setModel(WalletModel *model)
{
    this->model = model;
}
Arbitrage::~Arbitrage()
{
    delete ui;
}
