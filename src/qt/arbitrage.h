#ifndef ARBITRAGE_H
#define ARBITRAGE_H

#include <QDialog>
#include <QObject>
#include <stdint.h>
#include "ui_arbitrage.h"
#include "clientmodel.h"
#include "walletmodel.h"
#include "init.h"

#include <boost/filesystem.hpp>
#include <boost/filesystem/fstream.hpp>

#include <QJsonObject>
#include <QJsonArray>

namespace Ui {
class Arbitrage;
}
class WalletModel;

class Arbitrage : public QDialog
{
    Q_OBJECT

public:
    explicit Arbitrage(QWidget *parent = 0);
    ~Arbitrage();

    void setModel(WalletModel *model);

private slots:

    void InitArbitrage();
    void on_ArbitrageTabWidget_tabBarClicked(int index);
    void UpdaterFunction();
    void DisplayBalance(QLabel &BalanceLabel,QLabel &Available, QLabel &Pending, QString Currency,QString Response);
    void DisplayExtradeBalance(QLabel &BalanceLabel,QLabel &Available, QLabel &Pending, QString Currency,QString Response);
    void DisplaySafecexBalance(QLabel &BalanceLabel,QLabel &Available, QLabel &Pending, QString Currency,QString Response);
    void ActionsOnSwitch(int index);
    void BittrexToggled(bool checked);
    void SafecexToggled(bool checked);
    void ExtradeToggled(bool checked);

    // Save/Load Keys
    void on_UpdateKeys_clicked(bool Save=false, bool Load=false);
    void on_LoadKeys_clicked();
    void on_SaveKeys_clicked();
    string encryptDecrypt(string toEncrypt, string password);
    // Save/Load Keys

    // Arbitrage functions
    void StartArbitrage();
    void UpdateArbTable(QString exchange, double amount, double price, bool sell);
    void on_StartArbButton_clicked();
    void on_StopArbButton_clicked();
    void BuyExtradeSellBittrex(QJsonObject extrade, QJsonObject bittrex);
    void BuyBittrexSellExtrade(QJsonObject extrade, QJsonObject bittrex);
    void BuySafecexSellBittrex(QJsonObject safecex, QJsonObject bittrex);
    void BuyBittrexSellSafecex(QJsonObject safecex, QJsonObject bittrex);
    //void BuyExtradeSellSafeCex();
    //void BuySafeCexSellExtrade();
    // Arbitrage functions

    // Bittrex
    void on_BittrexTXGenDepositBTN_clicked();
    void on_BittrexBTCGenDepositBTN_clicked();
    void on_Bittrex_Withdraw_MaxTX_Amount_clicked();
    void on_Bittrex_Withdraw_MaxBTC_Amount_clicked();
    void on_BittrexWithdrawTXBtn_clicked();
    void on_BittrexWithdrawBTCBtn_clicked();
    void BittrexBuy(QJsonObject bittrex, QJsonObject extrade);
    //void BittrexSell(QJsonObject bittrex, QJsonObject extrade);
    void BittrexBuy2(QJsonObject bittrex, QJsonObject safecex);
    void BittrexSell2(QJsonObject bittrex, QJsonObject safecex);
    QString BuyTXBittrex(double Quantity, double Rate);
    QString SellTXBittrex(double Quantity, double Rate);
    QString BittrexWithdraw(double Amount, QString Address, QString Coin);
    QString GetBittrexOrderBook();
    QString GetBittrexBalance(QString Currency);
    QString GetBittrexTXAddress();
    QString GetBittrexBTCAddress();
    QString sendBittrexRequest(QString url, bool post=true);
    // Bittrex

    //Safecex
    void on_SafecexTXGenDepositBTN_clicked();
    void on_SafecexBTCGenDepositBTN_clicked();
    void on_Safecex_Withdraw_MaxTX_Amount_clicked();
    void on_Safecex_Withdraw_MaxBTC_Amount_clicked();
    void on_SafecexWithdrawTXBtn_clicked();
    void on_SafecexWithdrawBTCBtn_clicked();
    void SafecexBuy(QJsonObject bittrex, QJsonObject safecex);
    void SafecexSell(QJsonObject bittrex, QJsonObject safecex);
    //void SafecexBuy2(QJsonObject extrade, QJsonObject safecex);
    //void SafecexSell2(QJsonObject extrade, QJsonObject safecex);
    QString BuyTXSafecex(double Quantity, double Rate);
    QString SellTXSafecex(double Quantity, double Rate);
    QString SafecexWithdraw(double Amount, QString Address, QString Coin);
    QString GetSafecexOrderBook();
    QString GetSafecexBalance(QString Currency);
    QString GetSafecexTXAddress();
    QString GetSafecexBTCAddress();
    QString sendSafecexRequest(QString url, bool post=true);
    // Safecex

    // Extrade
    void on_ExtradeTXGenDepositBTN_clicked();
    void on_ExtradeBTCGenDepositBTN_clicked();
    void on_Extrade_Withdraw_MaxTX_Amount_clicked();
    void on_Extrade_Withdraw_MaxBTC_Amount_clicked();
    void on_ExtradeWithdrawTXBtn_clicked();
    void on_ExtradeWithdrawBTCBtn_clicked();
    QString BuyTXExtrade(QString OrderType, QString OrderSide, double Quantity, double Rate);
    void ExtradeBuy(QJsonObject bittrex, QJsonObject extrade);
    //QJsonObject ExtradeSell(QJsonObject bittrex, QJsonObject extrade);
    //QJsonObject ExtradeBuy2(QJsonObject extrade, QJsonObject safecex);
    //QJsonObject ExtradeSell2(QJsonObject extrade, QJsonObject safecex);
    QString SellTXExtrade(QString OrderType, QString OrderSide, double Quantity, double Rate);
    QString ExtradeWithdraw(double Amount, QString Address, QString Coin);
    QString GetExtradeOrderBook();
    QString GetExtradeBalance(QString Currency);
    QString GetExtradeTXAddress();
    QString GetExtradeBTCAddress();
    QString sendExtradeRequest(QString url, bool post);
    // Extrade

    QString BittrexTimeStampToReadable(QString DateTime);
    QString HMAC_SHA512_SIGNER(QString UrlToSign,QString Secretkey);
    QString HMAC_SHA256_SIGNER(QString UrlToSign,QString Secretkey);
    QJsonObject GetResultObjectFromJSONObject(QString response);
    QJsonObject GetResultObjectFromJSONArray(QString response);
    QJsonArray  GetResultArrayFromJSONObject(QString response);

public slots:


private:
    Ui::Arbitrage *ui;
    //Socket *socket;
    int timerid;
    int arbtimerid;
    double TotalEarned;
    bool EnableBittrex;
    bool EnableSafecex;
    bool EnableExtrade;
    QTimer *timer;
    QTimer *arbtimer;
    QString BittrexApiKey;
    QString BittrexSecretKey;
    QString SafecexApiKey;
    QString SafecexSecretKey;
    QString ExtradeApiKey;
    QString ExtradeSecretKey;
    WalletModel *model;


};

#endif // ARBITRAGE_H
