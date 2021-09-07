#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QMessageBox>
#include <QSettings>
#include <QDateTime>
#include <QFile>
#include <QDebug>
#include <QDir>
#include "settingsdialog.h"
#include "wavfile.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow),
    speakers(50),
    sound(synchro, sd),
    mainloop(synchro, sd, speakers)
{
    QLocale::setDefault(QLocale::English);

    QCoreApplication::setOrganizationName("Victor X");
    QCoreApplication::setOrganizationDomain("victorx.eu");
    QCoreApplication::setApplicationName("Acoustic Localization");
    QSettings::setDefaultFormat(QSettings::IniFormat);

    ui->setupUi(this);
    mainloop.setDebug(debug);
    ui->actionDebug->setChecked(debug);

    qDebug() << QDir::currentPath();

    connect(&sound, SIGNAL(info(QString)), this, SLOT(soundInfo(QString)));
    connect(&mainloop, SIGNAL(info(QString)), this, SLOT(soundInfo(QString)));
    connect(&mainloop, SIGNAL(correlation(const float*,int,int,int,int)), this,
            SLOT(correlation(const float*,int,int,int,int)), Qt::DirectConnection);
}

MainWindow::~MainWindow()
{
    if (ui->actionRun->isChecked())
    {
        sound.stop();
        mainloop.requestInterruption();
        synchro.setStopped(true);
        mainloop.wait();
    }
    delete ui;
}

void MainWindow::initSound()
{
    ui->inputChSpinBox->setMaximum(sd.inputs - 1);
    ui->outputChSpinBox->setMaximum(sd.outputs - 1);
    if (!sound.selectDevices(adcIn, dacOut, mainloop.sd.inputs, mainloop.sd.outputs, mainloop.sampling))
    {
        ui->actionRun->setEnabled(false);
    }
    ui->textBrowser->append("Sound device: " + sound.error());
}

void MainWindow::loadWave()
{
    WavFile wf;
    if (wf.load(fname, sd.ping))
    {
        fmt = wf.fileFormat();
        sd.outputs = fmt.channelCount();
        sd.frames = sd.ping.size() / sd.outputs;

        ui->statusBar->showMessage(QString::number(sd.ping.size()) + " samples");

        if (fmt.channelCount() != speakers.lastSpeaker + 1)
        {
            ui->textBrowser->append("WARNING Number of configured speakers " +
                                    QString::number(speakers.lastSpeaker + 1)
                                    + " != " + QString::number(fmt.channelCount()));
        }

        mainloop.sampling = fmt.sampleRate();
        if (mainloop.sampling != SAMPLE_RATE_)
        {
            ui->textBrowser->append("WARNING Sampling rate " +
                                    QString::number(mainloop.sampling)
                                    + " != " + QString::number(SAMPLE_RATE_));
        }

        double dur = (double)sd.frames / mainloop.sampling;
        if (dur > duration)
        {
            dur = duration;
        }
        // the part of used sound might be shorter than everything loaded from file
        sd.frames = dur * mainloop.sampling;
        sd.szOut = sd.frames * sd.outputs;
        sd.szIn = sd.frames * mainloop.sd.inputs;

        sd.ping.resize(sd.frames * sd.outputs);
        sd.pong.resize(sd.szIn);
        sd.bang.resize(sd.szIn);
        fadeInOutEx();

        ui->graph1->setData(sd.ping, fmt.channelCount(), -1);

        for (int i = 0; i < sd.outputs; i++)
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
        ui->actionDebug->setEnabled(false);
        ui->actionRun->setEnabled(false);
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
                             + "Build: " + built
                             + "\nData " + sd.toString());
}

void MainWindow::on_actionRun_toggled(bool active)
{
    if (active)
    {
        if (sound.start())
        {
            sound.debug = debug;
            ui->textBrowser->clear();
            ui->actionRun->setText("Stop");
            ui->actionRun->setIcon(QIcon(":/images/images/player_stop.png"));
            ui->statusBar->showMessage("Running");
            mainloop.allocateAll();
            mainloop.start();
        }
        else
        {
            ui->actionRun->setChecked(false);
            ui->statusBar->showMessage("Failed to start", 3000);
            ui->textBrowser->append(sound.error());
        }
    }
    else
    {
        sound.stop();
        mainloop.requestInterruption();
        synchro.setStopped(true);
        mainloop.wait();
        ui->actionRun->setText("Run");
        ui->actionRun->setIcon(QIcon(":/images/images/player_play.png"));
        ui->statusBar->showMessage("Stopped");
        if (debug)
        {
            WavFile vf(this);
            vf.save(pulsename, fmt, sd.ping); // output

            QAudioFormat afmt = fmt;
            afmt.setChannelCount(sd.inputs);
            vf.save(recname, afmt, sd.bang); // input
        }
    }
}

void MainWindow::on_actionExit_triggered()
{
    close();
}

void MainWindow::on_actionSettings_triggered()
{
    if (ui->actionRun->isChecked())
    {
        ui->actionRun->setChecked(false);
    }

    QFile f(m_cfg);
    if (f.open(QIODevice::ReadWrite))
    {
        QByteArray all = f.readAll();
        SettingsDialog dlg;
        dlg.setText(all);
        if (dlg.exec() == QDialog::Accepted)
        {
            f.reset();
            f.resize(0);
            f.write(dlg.getText().toLatin1());
            f.close();
            loadConfiguration(m_cfg.toLatin1().data());
            loadWave();
            initSound();
            ui->graph1->update();
        }
    }
}

void MainWindow::loadConfiguration(const char *cfg)
{
    QFile f(m_cfg = cfg);
    if (f.open(QIODevice::ReadOnly))
    {
        QString buf;
        while ( !(buf = f.readLine()).isEmpty() )
        {
            QStringList ss = buf.split("=", QString::SkipEmptyParts);
            if (ss.size() != 2) continue;
            QString ss0 = ss[0].trimmed();
            QString ss1 = ss[1].trimmed();
            if (ss0 == "duration")
            {
                duration = ss1.toDouble();
                qDebug() << "duration " << duration;
            }
            else if (ss0 == "file")
            {
                fname = ss1; //strdup(ofFilePath::getAbsolutePath(strtok(0, "\r\n")).c_str());
                qDebug() << "file name " << fname;
            }
            else if (ss0 == "recname")
            {
                recname = ss1; //strdup(ofFilePath::getAbsolutePath(strtok(0, "\r\n")).c_str());
                qDebug() << "record file name " << recname;
            }
            else if (ss0 == "pulsename")
            {
                pulsename = ss1; //strdup(ofFilePath::getAbsolutePath(strtok(0, "\r\n")).c_str());
                qDebug() << "file name " << pulsename;
            }
            else if (ss0 == "inputs")
            {
                mainloop.sd.inputs = ss1.toInt(); //;
                qDebug() << "inputs " << mainloop.sd.inputs << " " << sd.inputs;
            }
            else if (ss0 == "quality")
            {
                mainloop.qual = ss1.toFloat();
                qDebug() << "quality " << mainloop.qual;
            }
            else if (ss0 == "oscIP")
            {
                mainloop.oscIP = ss1.trimmed();
            }
            else if (ss0 == "oscPort")
            {
                mainloop.oscPort = ss1;
            }
            else if (ss0 == "adcIn")
            {
                adcIn = ss1;
                qDebug() << "adcIn " << adcIn;
            }
            else if (ss0 == "refIn")
            {
                mainloop.ref_in = ss1.toInt();
                qDebug() << "refIn " << mainloop.ref_in;
            }
            else if (ss0 == "refOut")
            {
                mainloop.ref_out = ss1.toInt();
                qDebug() << "refOut " << mainloop.ref_out;
            }
            else if (ss0 == "dacOut")
            {
                dacOut = ss1;
                qDebug() << "dacOut " << dacOut;
            }
            else if (ss0 == "debug")
            {
                debug = ss1.toInt();
                qDebug() << "debug " << debug;
            }
            else if (ss0 == "run")
            {
                run = ss1.toInt() != 0;
                qDebug() << "run " << run;
            }
            else if (ss0 == "threads")
            {
                mainloop.useThreads = ss1.toInt();
                qDebug() << "threads " << mainloop.useThreads;
            }
            else if (ss0 == "autopan")
            {
                mainloop.autopan = ss1.toInt() != 0;
                qDebug() << "autopan " << mainloop.autopan;
            }
            else if (ss0 ==  "fadems")
            {
                fadems = ss1.toDouble();
                qDebug() << "fadems " << fadems;
            }
            else if (ss0 == "pausems")
            {
                pausems = ss1.toDouble();
                qDebug() << "pausems " << pausems;
            }
            else if (ss0 == "pulsems")
            {
                pulsems = ss1.toDouble();
                qDebug() << "pulsems " << pulsems;
            }
            else if (ss0 ==  "pulsenumber")
            {
                pulsenumber = ss1.toInt();
                qDebug() << "pulsenumber " << pulsenumber;
            }
            else if (ss0 ==  "maxdist")
            {
                maxdist = ss1.toDouble();
                qDebug() << "maxdist " << maxdist;
            }
            else if (ss0 ==  "offsets")
            {
                QStringList oo = ss1.split(",");
                for (int i = 0; (i < MAX_OUTPUTS) && i < oo.size(); ++i)
                {
                    offsets[i] = oo[i].toDouble();
                    qDebug() << "offset " << i << " " << offsets[i];
                }
            }
            else if (ss0.mid(0, 7) == "speaker")
            {
                int spNum = ss0.mid(7).toInt();
                if (spNum >= 0 && spNum < MAX_OUTPUTS)
                {
                    QStringList oo = ss1.split(",", QString::SkipEmptyParts);
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
    else
    {
        ui->textBrowser->append("Not found " + m_cfg);
    }

    if (!mainloop.autopan)
    {
        speakers.findSpeakerPairs();
        ui->textBrowser->append(speakers.toString().c_str());
    }
}

void MainWindow::zeroChannel(int ch, long from, long to)
{
    int nch = sd.outputs;
    for (long i = from, idx = ch + nch * from; i < to; i++, idx += nch)
    {
        sd.ping[idx] = 0;
    }
}

void MainWindow::pulseChannel(int ch, long fade, long from, long to)
{
    int nch = sd.outputs;
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

    int nch = sd.outputs;
    long pls = pulsems / 1000.0 * mainloop.sampling;
    long pau = pausems / 1000.0 * mainloop.sampling;
    long period = pls + pau;
    long cnt = sd.frames;
    for (int ch = 0; ch < nch; ch++)
    {
        long offset = offsets[ch] / 1000.0 * mainloop.sampling;
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
            pulseChannel(ch, fadems / 1000.0 * mainloop.sampling, offset, end);
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

void MainWindow::correlation(const float *res, int sz, int inp, int outp, int idx)
{
    // Don't waste time
    if (ui->disableCheckBox->isChecked()) return;

    if (inp == ui->inputChSpinBox->value() &&
            outp == ui->outputChSpinBox->value())
    {
        ui->graphCorrx->setData(res, sz, 1, idx);
        ui->graphCorrx->update();
    }
}

void MainWindow::soundInfo(QString info)
{
    // Don't waste time
    if (ui->disableCheckBox->isChecked()) return;

    if (info.startsWith("debug "))
    {
        if (debug)
        {
            on_actionDebug_triggered();
        }
        ui->statusBar->showMessage(info.mid(6));
        ui->textBrowser->append(info.mid(6));
    }
    else
    {
        ui->textBrowser->append(info);
    }
}

void MainWindow::on_actionDebug_triggered()
{
    debug = ui->actionDebug->isChecked();
    mainloop.setDebug(debug);
    sound.debug = debug;
    if (debug)
    {
        ui->graph2->setData(sd.bang, sd.inputs, -1);
    }
    else
    {
        ui->graph2->setData(sd.bang, 0, -1);
    }
    ui->graph2->update();
}

void MainWindow::on_disableCheckBox_toggled(bool checked)
{
    ui->textBrowser->setEnabled(!checked);
    ui->statusBar->showMessage(checked ? "GUI Muted" : "GUI Unmuted", 3000);
}
