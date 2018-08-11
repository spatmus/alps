#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtMultimedia/qaudioformat.h>
// #include "audiofileloader.h"
#include "Speakers.hpp"
#include "Synchro.hpp"
#include "sounddevice.h"

namespace Ui {
class MainWindow;
}

#define DURATION    0.5
#define MAX_OUTPUTS 16
#define MAX_INPUTS  16
#define ADC_INPUTS  2
#define SAMPLE_RATE 96000
#define REPEAT      20
#define QUALITY     0.0
#define OSC_IP      "192.168.1.4"
#define OSC_PORT    7770

typedef float SAMPLE;

struct SoundData {
    long        szIn;
    long        szOut;
    std::vector<SAMPLE>      ping;
    std::vector<SAMPLE>      pong;
    std::vector<SAMPLE>      bang;
    int         channels;
    int         frames;
    int         empty;
};

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void loadConfiguration(const char *cfg);
    void loadWave();
    void initSound();

private slots:
    void loaded();

    void on_actionRun_toggled(bool active);

    void on_actionExit_triggered();

    void on_actionAbout_triggered();

    void on_actionSettings_triggered();

private:
    Ui::MainWindow *ui;

    QAudioFormat    fmt;
//    AudioFileLoader loader;
    Speakers        speakers = Speakers(50);
    Synchro         synchro;
    SoundData       sd;
    SoundDevice     sound;

    QString adcIn = "Built-in Input";
    QString dacOut = "Built-in Output";

    QString fname = "noise.wav";
    QString recname = "record.wav";
    QString pulsename = "pulses.wav";

    QString oscIP = OSC_IP;
    int oscPort = OSC_PORT;
    // ofxOscSender osc;

    int inputs = 2; // stereo
    int outputs = 2;
    int ref_out = 0;
    int ref_in = 0;

    double duration = DURATION;

    // the measurement result
    int delays[MAX_OUTPUTS][MAX_INPUTS];

    int repeat = REPEAT;
    float qual = QUALITY;

    bool autopan = false;
    bool debug = false;
    bool run = false;

    // values loaded from configuration file
    // values in milliseconds for advanced fading
    double fadems = 5;    // fade in and fade out in each pulse
    double pausems = 10;  // pause between pulses
    double pulsems = 40;  // pulse duration
    double offsets[MAX_OUTPUTS] = { 0, 10, 20, 30, 40, 50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150};
    int pulsenumber = 5;

    double maxdist = 100;

    int compute(); // returns reference delay in samples
    void report();
    bool distToXY(float d1, float d2, float L, float *x, float *y);
    bool distToXY(float d1, float d2, int n, int m, float *x, float *y);
    double pythagor(double d, double z);
    bool distOk(float &d, int n, float z);

    void fadeInOutEx();
    void zeroChannel(int ch, long from, long to);
    void pulseChannel(int ch, long fade, long from, long to);
};

#endif // MAINWINDOW_H
