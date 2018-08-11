#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include "settingsdialog.h"
#include "wavfile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    QLocale::setDefault(QLocale::English);

    QCoreApplication::setOrganizationName("Victor X");
    QCoreApplication::setOrganizationDomain("victorx.eu");
    QCoreApplication::setApplicationName("Acoustic Localization");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initSound()
{
    if (!sound.selectDevices(adcIn, dacOut, inputs, outputs, SAMPLE_RATE))
    {
        ui->textBrowser->append("Sound device: " + sound.error());
    }
    else
    {
        ui->textBrowser->append("Sound device: " + sound.error());
    }
}

void MainWindow::loadWave()
{
    //    connect(&loader, SIGNAL(done()), this, SLOT(loaded()));
    //    loader.readFile(fname);
    WavFile wf;
    if (wf.load(fname, sd.ping))
    {
        fmt = wf.fileFormat();
        sd.channels = fmt.channelCount();
        sd.frames = sd.ping.size() / sd.channels;

        ui->statusBar->showMessage(QString::number(sd.ping.size()) + " samples");

        if (fmt.channelCount() != speakers.lastSpeaker + 1)
        {
            ui->textBrowser->append("WARNING Number of configured speakers " +
                                    QString::number(speakers.lastSpeaker + 1)
                                    + " != " + QString::number(fmt.channelCount()));
        }

        float dur = (float)sd.frames / SAMPLE_RATE;
        if (dur > duration)
        {
            dur = duration;
        }
        // the part of used sound might be shorter than everything loaded from file
        sd.frames = dur * SAMPLE_RATE;
        sd.szOut = sd.frames * sd.channels;
        sd.szIn = sd.frames * inputs;
        fadeInOutEx();

        ui->graph1->setData(sd.ping, fmt.channelCount(), sd.frames);

        for (int i = 0; i < sd.channels; i++)
        {
            xyz &co = speakers.getCoordinates(i);
            ui->textBrowser->append("[" + QString::number(co.x) + ", " +
                                    QString::number(co.y) + ", " +
                                    QString::number(co.z) + "]");
        }

    }
    else
    {
        ui->statusBar->showMessage("Not loaded: " + fname);
    }
}

void MainWindow::loaded()
{
//    ui->statusBar->showMessage("Decoded audio file, " +
//                               QString::number(loader.buffer.size()) + " bytes");
}

void MainWindow::on_actionAbout_triggered()
{
    QStringList ss = QString(__DATE__).split(" ", QString::SkipEmptyParts);
    QString s = ss[0] + " " + ss[1] + " " + ss[2] + " " __TIME__;
    QLocale::setDefault(QLocale::English);
    QDateTime dt = QLocale().toDateTime(s, "MMM d yyyy hh:mm:ss");
    QString built = dt.isValid() ? dt.toString("yyMM.ddhh.mm") : s;

    QMessageBox::information(this, QApplication::applicationName(),
                             QApplication::organizationName() + QString::fromWCharArray(L" \u00a9 2018\n")
                             + "Build: " + built);
}

void MainWindow::on_actionRun_toggled(bool active)
{
    QString txt = active ? "Stop" : "Run";
    ui->actionRun->setText(txt);
    if (txt == "Run") txt = ":/images/images/player_play.png";
    else if (txt == "Stop") txt = ":/images/images/player_stop.png";
    ui->actionRun->setIcon(QIcon(txt));
    ui->statusBar->showMessage(active ? "Running" : "Stopped");
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionSettings_triggered()
{
    SettingsDialog dlg;
    dlg.exec();
}

void MainWindow::loadConfiguration(const char *cfg)
{
    QFile f(cfg);
    if (f.open(QIODevice::ReadOnly))
    {
        QString buf;
        while ( !(buf = f.readLine()).isEmpty() )
        {
            QStringList ss = buf.split("=", QString::SkipEmptyParts);
            if (ss.size() != 2) continue;
            if (ss[0] == "duration")
            {
                duration = ss[1].toDouble();
                qDebug() << "duration " << duration;
            }
            else if (ss[0] == "file")
            {
                fname = ss[1].trimmed(); //strdup(ofFilePath::getAbsolutePath(strtok(0, "\r\n")).c_str());
                qDebug() << "file name " << fname;
            }
            else if (ss[0] == "recname")
            {
                recname = ss[1].trimmed(); //strdup(ofFilePath::getAbsolutePath(strtok(0, "\r\n")).c_str());
                qDebug() << "record file name " << recname;
            }
            else if (ss[0] == "pulsename")
            {
                pulsename = ss[1].trimmed(); //strdup(ofFilePath::getAbsolutePath(strtok(0, "\r\n")).c_str());
                qDebug() << "file name " << pulsename;
            }
            else if (ss[0] == "inputs")
            {
                inputs = ss[1].toInt(); //;
                qDebug() << "inputs " << inputs;
            }
            else if (ss[0] == "repeat")
            {
                repeat = ss[1].toInt();
                qDebug() << "repeat " << repeat;
            }
            else if (ss[0] == "quality")
            {
                qual = ss[1].toFloat();
                qDebug() << "quality " << qual;
            }
            else if (ss[0] == "oscIP")
            {
                oscIP = ss[1].trimmed();
                qDebug() << "oscIP " << oscIP;
            }
            else if (ss[0] == "oscPort")
            {
                oscPort = ss[1].toInt();
                qDebug() << "oscPort " << oscPort;
            }
            else if (ss[0] == "adcIn")
            {
                adcIn = ss[1].trimmed();
                qDebug() << "adcIn " << adcIn;
            }
            else if (ss[0] == "refIn")
            {
                ref_in = ss[1].toInt();
                qDebug() << "refIn " << ref_in;
            }
            else if (ss[0] == "refOut")
            {
                ref_out = ss[1].toInt();
                qDebug() << "refOut " << ref_out;
            }
            else if (ss[0] == "dacOut")
            {
                dacOut = ss[1].trimmed();
                qDebug() << "dacOut " << dacOut;
            }
            else if (ss[0] == "debug")
            {
                debug = ss[1].toInt();
                qDebug() << "debug " << debug;
            }
            else if (ss[0] == "run")
            {
                run = ss[1].toInt() != 0;
                qDebug() << "run " << run;
            }
            else if (ss[0] == "autopan")
            {
                autopan = ss[1].toInt() != 0;
                qDebug() << "autopan " << autopan;
            }
            else if (ss[0] ==  "fadems")
            {
                fadems = ss[1].toDouble();
                qDebug() << "fadems " << fadems;
            }
            else if (ss[0] == "pausems")
            {
                pausems = ss[1].toDouble();
                qDebug() << "pausems " << pausems;
            }
            else if (ss[0] == "pulsems")
            {
                pulsems = ss[1].toDouble();
                qDebug() << "pulsems " << pulsems;
            }
            else if (ss[0] ==  "pulsenumber")
            {
                pulsenumber = ss[1].toInt();
                qDebug() << "pulsenumber " << pulsenumber;
            }
            else if (ss[0] ==  "maxdist")
            {
                maxdist = ss[1].toDouble();
                qDebug() << "maxdist " << maxdist;
            }
            else if (ss[0] ==  "offsets")
            {
                QStringList oo = ss[1].split(",");
                for (int i = 0; (i < MAX_OUTPUTS) && i < oo.size(); ++i)
                {
                    offsets[i] = oo[i].toDouble();
                    qDebug() << "offset " << i << " " << offsets[i];
                }
            }
            else if (ss[0].mid(0, 7) == "speaker")
            {
                int spNum = ss[0].mid(7).toInt();
                if (spNum >= 0 && spNum < MAX_OUTPUTS)
                {
                    QStringList oo = ss[1].split(",", QString::SkipEmptyParts);
                    if (oo.size() == 3)
                    {
                        int idx = spNum;
                        double x = oo[0].toDouble();
                        double y = oo[1].toDouble();
                        double z = oo[2].toDouble();
                        xyz sp(x, y, z);
                        if (!speakers.set(idx, sp))
                        {
                            ui->textBrowser->append("NOTE: speaker " + QString::number(spNum) +
                                                    ": IGNORED " + buf);
                        }
                    }
                }
                else
                {
                    ui->textBrowser->append("NOTE: IGNORED " + buf);
                }
            }
        }
    }
}

void MainWindow::zeroChannel(int ch, long from, long to)
{
    int nch = sd.channels;
    for (long i = from, idx = ch + nch * from; i < to; i++, idx += nch)
    {
        sd.ping[idx] = 0;
    }
}

void MainWindow::pulseChannel(int ch, long fade, long from, long to)
{
    int nch = sd.channels;
    for (long i = 0, idx1 = ch + nch * from, idx2 = nch * to + ch; i < fade;
         i++, idx1 += nch, idx2 -= nch)
    {
        float v = (float) i / fade;
        sd.ping[idx1] *= v;
        sd.ping[idx2] *= v;
    }
}

void MainWindow::fadeInOutEx()
{
    // check if parameters make sense
    if (fadems * 2 > pulsems)
    {
        // std::cout << "ERROR: fadems " << fadems << " is too big, should not be greater than pulsems " << pulsems << std::endl;
        return;
    }

    int nch = sd.channels;
    long pls = pulsems / 1000.0 * SAMPLE_RATE;
    long pau = pausems / 1000.0 * SAMPLE_RATE;
    long period = pls + pau;
    long cnt = sd.frames;
    for (int ch = 0; ch < nch; ch++)
    {
        long offset = offsets[ch] / 1000.0 * SAMPLE_RATE;
        zeroChannel(ch, 0, offset);

        if (debug)
            ui->textBrowser->append("pulse " + QString::number(pls) + " pause "
                                    + QString::number(pau) + " ofs "
                                    + QString::number(offset));

        for (int i = 0; i < pulsenumber && offset < cnt; i++)
        {
            long end = offset + pls;
            if (end >= cnt) end = cnt - 1;
            // if (debug) cout << "ch " << ch << " pls " << i << " ofs " << offset << " end " << end << endl;
            pulseChannel(ch, fadems / 1000.0 * SAMPLE_RATE, offset, end);
            long end2 = end + pau;
            if (end2 > cnt) end2 = cnt;
            zeroChannel(ch, end, end2);
            offset += period;
        }

        if (offset < cnt)
        {
            zeroChannel(ch, offset, cnt);
        }
    }
}

/*
 *     SoundData &d = *((SoundData*)userData);
    int ptr = lckr.getAudioPtr();
    if (ptr == 0)
    {
        memcpy(d.bang, d.pong, sizeof(SAMPLE) * d.sfInfo.frames * inputs);
        lckr.allowCompute();
    }

    if (ptr < d.sfInfo.frames)
    {
        long long frames = d.sfInfo.frames - ptr;
        if (frames > framesPerBuffer)
        {
            frames = framesPerBuffer;
        }
        long pOut = ptr * d.sfInfo.channels;
        long pIn = ptr * inputs;
        memcpy(outputBuffer, d.ping + pOut, sizeof(SAMPLE) * frames * d.sfInfo.channels);
        memcpy(d.pong + pIn, inputBuffer, sizeof(SAMPLE) * frames * inputs);
        lckr.addAudioPtr((int)frames, d.sfInfo.frames);
    }
    else
    {
        memset(outputBuffer, 0, sizeof(SAMPLE) * framesPerBuffer * d.sfInfo.channels);
    }
    return paContinue;
*/

