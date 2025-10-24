/*
 * @Author: wuxiaoxiao
 * @Email: wuxiaoxiao@gmail.com
 * @Date: 2025-10-24 11:00:39
 * @LastEditors: wuxiaoxiao
 * @LastEditTime: 2025-10-24 21:06:35
 * @Description: 
 */
// 这个文件包含需要添加到mainoverlayout.cpp末尾的槽函数实现

// ============ 雷达控制槽函数实现 ============

/**
 * @brief 打开一键全数配置对话框
 * @details 配置阵地控制参数
 */
void MainOverLayOut::onBatteryControlClicked()
{
    BatteryControl* dialog = new BatteryControl(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &BatteryControl::setParam, CON_INS, &Controller::sendBCParam);
}

/**
 * @brief 打开处理软件启动对话框
 * @details 发送系统启动命令
 */
void MainOverLayOut::onStartSoftwareClicked()
{
    if (CustomMessageBox::showConfirm(this, "启动确认", "是否启动处理软件？")) {
        StartSysParam param;
        // 设置默认参数
        param.startSta = 1;  // 启动状态
        emit CON_INS->sendSysStart(param);
    }
}

/**
 * @brief 打开数据存储/删除对话框
 * @details 配置数据保存和删除参数
 */
void MainOverLayOut::onDataStorageClicked()
{
    DataSaveUI* dialog = new DataSaveUI(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &DataSaveUI::setParam, CON_INS, &Controller::sendDSParam);
}

/**
 * @brief 打开发射接收控制对话框
 * @details 配置收发控制参数
 */
void MainOverLayOut::onTransmitControlClicked()
{
    TranRecvUI* dialog = new TranRecvUI(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &TranRecvUI::setParam, CON_INS, &Controller::sendTRParam);
}

// ============ 参数设置槽函数实现 ============

/**
 * @brief 打开数据处理参数对话框
 * @details 配置数据处理相关参数
 */
void MainOverLayOut::onDataProcessClicked()
{
    DataProcessUI* dialog = new DataProcessUI(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &DataProcessUI::setParam, CON_INS, &Controller::sendDPParam);
}

/**
 * @brief 打开信号处理参数对话框
 * @details 配置信号处理相关参数
 */
void MainOverLayOut::onSignalProcessClicked()
{
    SigParamUI* dialog = new SigParamUI(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &SigParamUI::setParam, CON_INS, &Controller::sendSPParam);
}

/**
 * @brief 打开波形及采样控制对话框
 * @details 配置频率控制和波形参数
 */
void MainOverLayOut::onFreqControlClicked()
{
    // 创建一个Tab对话框包含两个子对话框
    QDialog* dialog = new QDialog(this);
    dialog->setWindowTitle("波形及采样控制");
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->resize(600, 500);

    QVBoxLayout* layout = new QVBoxLayout(dialog);
    QTabWidget* tabWidget = new QTabWidget(dialog);

    // 频率控制页面
    FreqControlUI* freqControl = new FreqControlUI(dialog);
    tabWidget->addTab(freqControl, "频率控制");
    connect(freqControl, &FreqControlUI::setParam, CON_INS, &Controller::sendFCParam);

    // 波形采样页面
    WaveAndSample* waveControl = new WaveAndSample(dialog);
    tabWidget->addTab(waveControl, "波形采样");
    connect(waveControl, &WaveAndSample::setParam, CON_INS, &Controller::sendWCParam);

    layout->addWidget(tabWidget);
    dialog->show();
}

/**
 * @brief 打开电调控制对话框
 * @details 配置波束控制参数
 */
void MainOverLayOut::onBeamControlClicked()
{
    WaveAndSample* dialog = new WaveAndSample(this);
    dialog->setWindowTitle("电调控制");
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &WaveAndSample::setParam, CON_INS, &Controller::sendWCParam);
}

/**
 * @brief 打开方向图扫描控制对话框
 * @details 配置扫描范围参数
 */
void MainOverLayOut::onScanRangeClicked()
{
    ScanRangeUI* dialog = new ScanRangeUI(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接参数设置信号到Controller
    connect(dialog, &ScanRangeUI::setParam, CON_INS, &Controller::sendSRParam);
}

/**
 * @brief 打开光电系统控制对话框
 * @details 配置光电参数
 */
void MainOverLayOut::onPhotoelectricClicked()
{
    PhotoElectricParam* dialog = new PhotoElectricParam(this);
    dialog->setAttribute(Qt::WA_DeleteOnClose);
    dialog->show();

    // 连接光电参数信号到Controller
    connect(dialog, &PhotoElectricParam::sendPEParam, CON_INS, &Controller::sendPEParam);
    connect(dialog, &PhotoElectricParam::sendPEParam2, CON_INS, &Controller::sendPEParam2);
}
