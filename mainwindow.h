#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <qgraphicsscene.h>

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_startsimulation_clicked();
    void checkInputsReady();
    void drawCacheView(int cacheSize, int blockSize, int associativity);
    void on_nextStep_clicked();



private:
    Ui::MainWindow *ui;
    QGraphicsScene *cacheScene = nullptr;
    void drawDirectMapped(int cacheSize, int blockSize);
    void drawTwoWay(int cacheSize, int blockSize);
    void drawFourWay(int cacheSize, int blockSize);
    void drawFullyAssociative(int cacheSize, int blockSize);
    struct CacheLine {
        QString tag;
        QVector<uint8_t> data;
        int lineIndex;   // which line (for set associative)
        int lastaccess;
        int firstaccess;
    };
    QVector<QVector<CacheLine>> cache;  // cache[set][way]
    char mockData[2049] = "d6715e3304a49b5f8d9e4ce2d701f8ead6870a38a293f86484d42ebbb8349a42dfc52a33b89c4942e937ee027a4a4d7bad54ede2c1915aecf87a93e6c301342eb2a720ab1207aa71a0906be8b1c257f6955831aa7eabad68b0c1ee8559f84b9b65340cf4281544a8fe2533cd02aea9b7249816e996ff3494f0e332e444928beaadf8b471e167c8c713e60db7f08f047da0c487d13b9991f867d6944e360437fb60474b1067ec44edd5b5fd451fac8d2c74c6fc7330896cecc8f0aab6195b13d44e188cb425c7529255bd35baba18578b3a6a22ab4958998ab6ed5a6f464b73c5cd182b9b3f3cf405fab6e523037f50819804edee69e43aff9f738724f5f02f39515fda6610cbb823d213ac6d92a0566a9a21620cb0658f6fffe60a6579f5fc46ed5896b19b3feb3d950623d418c312d3b3200f9ca23ef20e0166815fbacfe230079bbf68575b80d65ca20b97398efcd1ab18719e564f0d2f4f1f2cff6ae2d52816db2a99525838b07f2fac6890822072b9efb664e0993625376221c723acabc3b2cbb2fff1398d2f82f7cbef02f4cdc551509e113022fc2862e7bfe5a47cdf74273a71a5ddb5b32e5b047e18ad647dd5ea62868f4be1a9c7c6f6aa9f147bf6ef1a158928f9c23427bee87763791a31ddb2e1c5a4fa7fd16e3f419c63aa99d0e95bdb26a85d36b9378c8c1f4ce6563516b228b57bd83e669502d0a2b4e1995263eebb22977f02487581ee97adf230c3eb9c22fe5358e3fc592f2a141e7403d4c366b40de892e1b20eff9713b7ede2789aeab994e83c41ee95be8cceb2c75ab80723dcbd31c967b9556856af77d911516e1c7bc6d2bff3598ece7ecacea5170785b1c900c8c77555940ca6eb09f69af1fc686743bef1b7d20706d683b99371d8bafdadeac9ef5ae78c1aa5347a6786093c5296675728b564895d4511fb7bbe2dc50f832d15d08c24a884f3a30fd012347f830bf761fd4f19e493885b57966ef579bde655d51907bbe5f079a6ffaef6268271ee5f92f68fecb7c2f095b1f73f2b3683365773f3614ea61e9e9c4d4b9ca545d2500d1c11dc194c7621c5692338c1eb8fae649f8a5cd7f1f4ea304552a364e24697612f803b05c0c60ab3824f7883a5f7a6f0a07b9fe657267256be8f297b322e2bbdf88003406eb437cf5541d79706da3f22c25cebee5e6b7d2dcf5f7ba937cc8ad325eac1a629e4a9331c7973f8cb5b93d1dde0673eb7d1c5854d8209d74dab645a0d8c464cc4bc45d3660a3fc0e2f2c13318441d327d95b27bc7d333f1c351ac4e76c6a555543ef603eb0ddfeae9054e833871ca1d0b5e69e3b3ae89609c91e0765ea0334698cc88be86df63cb90f8dd1b63b1b10289055bb48f246dc3c796be4ec168d9fc52fe4169700ed3ee77579e7233cd169d8657ec58f26c668f3b2dbc63e774815fc87a8f65a65c47990b";
    int currentBlockSize = 0;
    int currentCacheSize = 0;
    int currentAssociativity = 0;
    int currentInstructionLine = 0;
    int accessCounter = 0;
    int currentReplacementPolicy = 5;

    void updateCacheVisualization();
    int findReplacementWay_LRU(int setIndex);
    int findReplacementWay_FIFO(int setIndex);


};
#endif // MAINWINDOW_H
