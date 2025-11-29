#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include "MemoryWindow.h"

#include <QGraphicsRectItem>
#include <QGraphicsTextItem>
#include <cmath>
#include <iostream>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    // Populate cache size (in Bytes)
    ui->setupUi(this);
    ui->cachesize->addItem("4");
    ui->cachesize->addItem("8");
    ui->cachesize->addItem("16");
    ui->cachesize->addItem("64");
    ui->cachesize->addItem("128");

    // Populate associativity
    ui->asso->addItem("Direct-mapped", QVariant(1));      // 1-way
    ui->asso->addItem("2-way", QVariant(2));
    ui->asso->addItem("4-way", QVariant(4));
    ui->asso->addItem("Fully associative", QVariant(0)); // 0 -> special marker

    // Populate block size (bytes)
    ui->blocksize->addItem("4");
    ui->blocksize->addItem("8");
    ui->blocksize->addItem("16");
    ui->blocksize->addItem("32");

    //populate policy
    ui->replacement->addItem("LRU", QVariant(5));
    ui->replacement->addItem("FIFO", QVariant(6));

    // Initially disable Start Simulation button
    ui->startsimulation->setEnabled(false);

    // Connect combo box signals to slot to check if ready
    connect(ui->cachesize, &QComboBox::currentTextChanged,
            this, &MainWindow::checkInputsReady);
    connect(ui->asso, &QComboBox::currentTextChanged,
            this, &MainWindow::checkInputsReady);
    connect(ui->blocksize, &QComboBox::currentTextChanged,
            this, &MainWindow::checkInputsReady);

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_startsimulation_clicked()
{
    int blockSize = ui->blocksize->currentText().toInt();
    int cacheSize = ui->cachesize->currentText().toInt();
    int numofblocks = cacheSize/blockSize;
    //int associativity = ui->asso->currentText().toInt();
    int rawAssoc = ui->asso->currentData().toInt(); // 1,2,4 or -1
    int associativity;
    if (rawAssoc == 0) {                // -1 or 0 = "Fully associative"
        associativity = numofblocks;
    } else {
        associativity = rawAssoc;
    }
    // Validation: cacheSize >= blockSize
    if (cacheSize < blockSize) {
        ui->textBrowser->append(
            QString("Error: Cache size (%1 Bytes) must be >= Block size (%2 Bytes)")
                .arg(cacheSize).arg(blockSize)
            );
        return; // stop execution
    } else {
        ui->textBrowser->append(
            QString("Great ! Cache size (%1 Bytes) divided by Block size (%2 Bytes) equals (%3 Blocks)")
                .arg(cacheSize).arg(blockSize).arg(numofblocks)
            );
    }
    // Store current configuration
    currentBlockSize = blockSize;
    currentCacheSize = cacheSize;
    currentAssociativity = associativity;
    int currentReplacementPolicy = ui->replacement->currentData().toInt();
    currentInstructionLine = 0;
    accessCounter = 0;

    // If valid, proceed to open MemoryWindow
    MemoryWindow *mw = new MemoryWindow(blockSize, this);
    mw->setAttribute(Qt::WA_DeleteOnClose); // auto cleanup
    mw->show();
    //std::cout << associativity;
    //std::cout << associativity << "_";

    drawCacheView(cacheSize, blockSize, associativity);


}

void MainWindow::checkInputsReady()
{
    bool ready = !ui->cachesize->currentText().isEmpty() &&
                 !ui->asso->currentText().isEmpty() &&
                 !ui->blocksize->currentText().isEmpty();

    ui->startsimulation->setEnabled(ready);
}

void MainWindow::drawCacheView(int cacheSize, int blockSize, int associativity)
{
    switch (associativity)
    {
    case 1:     // Direct mapped
        drawDirectMapped(cacheSize, blockSize);
        break;

    case 2:     // 2-way associative
        drawTwoWay(cacheSize, blockSize);
        break;

    case 4:     // 4-way associative
        drawFourWay(cacheSize, blockSize);
        break;

    case 0:     // Fully associative
    default:
        drawFullyAssociative(cacheSize, blockSize);
        break;
    }
}

void MainWindow::drawDirectMapped(int cacheSize, int blockSize)
{
    // Direct-mapped: 1 way, multiple sets
    // Number of sets = cacheSize / blockSize
    int numSets = cacheSize / blockSize;

    // Initialize cache structure: numSets rows, 1 way each
    cache.clear();
    cache.resize(numSets);
    for (int set = 0; set < numSets; ++set) {
        cache[set].resize(1); // 1 way per set
        // Initialize the cache line
        cache[set][0].tag = "";
        cache[set][0].data.resize(blockSize);
        cache[set][0].data.fill(0);
        cache[set][0].lineIndex = 0;
        cache[set][0].lastaccess = -1;
        cache[set][0].firstaccess = -1;
    }

    // Create a scene if not already present
    if (!cacheScene) {
        cacheScene = new QGraphicsScene(this);
        ui->cacheView->setScene(cacheScene);
    }
    cacheScene->clear();

    const int cellWidth = 40;
    const int cellHeight = 40;
    const int tagWidth = 80;
    const int labelWidth = 80;

    QPen pen(Qt::black);
    pen.setWidth(1);

    // Calculate number of bits needed for byte offset and set index
    int offsetBits = static_cast<int>(std::log2(blockSize));
    int indexBits = static_cast<int>(std::log2(numSets));

    // Draw "TAG" header
    QGraphicsTextItem *tagHeader = cacheScene->addText("TAG");
    tagHeader->setPos(labelWidth + 20, -25);

    // Draw byte offset column headers (in binary)
    for (int byte = 0; byte < blockSize; ++byte) {
        QString bin = QString("%1").arg(byte, offsetBits, 2, QLatin1Char('0'));
        QGraphicsTextItem *txt = cacheScene->addText(bin);
        txt->setScale(0.8);
        txt->setPos(labelWidth + tagWidth + byte * cellWidth + 8, -25);
    }

    // Draw each set (row)
    for (int set = 0; set < numSets; ++set) {
        // Set label in binary
        QString setBin = QString("%1").arg(set, indexBits, 2, QLatin1Char('0'));
        QGraphicsTextItem *setLabel = cacheScene->addText(setBin);
        setLabel->setPos(0, set * cellHeight + 10);

        // Draw TAG cell
        QRectF tagRect(labelWidth, set * cellHeight, tagWidth, cellHeight);
        cacheScene->addRect(tagRect, pen);

        // Draw each byte cell in this block
        for (int byte = 0; byte < blockSize; ++byte) {
            QRectF cellRect(labelWidth + tagWidth + byte * cellWidth, set * cellHeight, cellWidth, cellHeight);
            cacheScene->addRect(cellRect, pen);
        }
    }

    cacheScene->setSceneRect(-10, -20, labelWidth + tagWidth + blockSize * cellWidth + 20, numSets * cellHeight + 20);
}

void MainWindow::drawTwoWay(int cacheSize, int blockSize)
{
    // 2-way set associative
    int numSets = cacheSize / blockSize / 2; // 2 ways
    int numWays = 2;

    // Initialize cache structure: numSets rows, 2 ways each
    cache.clear();
    cache.resize(numSets);
    for (int set = 0; set < numSets; ++set) {
        cache[set].resize(numWays);
        for (int way = 0; way < numWays; ++way) {
            cache[set][way].tag = "";
            cache[set][way].data.resize(blockSize);
            cache[set][way].data.fill(0);
            cache[set][way].lineIndex = way;
            cache[set][way].lastaccess = -1;
            cache[set][way].firstaccess = -1;
        }
    }

    if (!cacheScene) {
        cacheScene = new QGraphicsScene(this);
        ui->cacheView->setScene(cacheScene);
    }
    cacheScene->clear();

    const int cellWidth = 40;
    const int cellHeight = 40;
    const int tagWidth = 80;
    const int labelWidth = 80;

    QPen pen(Qt::black);
    pen.setWidth(1);

    // Calculate number of bits needed for byte offset and set index
    int offsetBits = static_cast<int>(std::log2(blockSize));
    int indexBits = static_cast<int>(std::log2(numSets));

    // Draw "TAG" header
    QGraphicsTextItem *tagHeader = cacheScene->addText("TAG");
    tagHeader->setPos(labelWidth + 20, -25);

    // Draw byte offset column headers (in binary)
    for (int byte = 0; byte < blockSize; ++byte) {
        QString bin = QString("%1").arg(byte, offsetBits, 2, QLatin1Char('0'));
        QGraphicsTextItem *txt = cacheScene->addText(bin);
        txt->setScale(0.8);
        txt->setPos(labelWidth + tagWidth + byte * cellWidth + 8, -25);
    }

    // Draw each set with 2 ways (2 rows per set)
    int currentRow = 0;
    for (int set = 0; set < numSets; ++set) {
        for (int way = 0; way < numWays; ++way) {
            // Set and way label in binary
            QString setBin = QString("%1").arg(set, indexBits, 2, QLatin1Char('0'));
            QString wayBin = QString("%1").arg(way, 1, 2, QLatin1Char('0'));
            QGraphicsTextItem *label = cacheScene->addText(setBin + ":" + wayBin);
            label->setPos(0, currentRow * cellHeight + 10);

            // Draw TAG cell
            QRectF tagRect(labelWidth, currentRow * cellHeight, tagWidth, cellHeight);
            cacheScene->addRect(tagRect, pen);

            // Draw each byte cell in this block
            for (int byte = 0; byte < blockSize; ++byte) {
                QRectF cellRect(labelWidth + tagWidth + byte * cellWidth, currentRow * cellHeight, cellWidth, cellHeight);
                cacheScene->addRect(cellRect, pen);
            }
            currentRow++;
        }
    }

    cacheScene->setSceneRect(-10, -20, labelWidth + tagWidth + blockSize * cellWidth + 20, currentRow * cellHeight + 20);
}

void MainWindow::drawFourWay(int cacheSize, int blockSize)
{
    // 4-way set associative
    int numSets = cacheSize / blockSize / 4; // 4 ways
    int numWays = 4;

    // Initialize cache structure: numSets rows, 4 ways each
    cache.clear();
    cache.resize(numSets);
    for (int set = 0; set < numSets; ++set) {
        cache[set].resize(numWays);
        for (int way = 0; way < numWays; ++way) {
            cache[set][way].tag = "";
            cache[set][way].data.resize(blockSize);
            cache[set][way].data.fill(0);
            cache[set][way].lineIndex = way;
            cache[set][way].lastaccess = -1;
            cache[set][way].firstaccess = -1;
        }
    }

    if (!cacheScene) {
        cacheScene = new QGraphicsScene(this);
        ui->cacheView->setScene(cacheScene);
    }
    cacheScene->clear();

    const int cellWidth = 40;
    const int cellHeight = 40;
    const int tagWidth = 80;
    const int labelWidth = 80;

    QPen pen(Qt::black);
    pen.setWidth(1);

    // Calculate number of bits needed for byte offset and set index
    int offsetBits = static_cast<int>(std::log2(blockSize));
    int indexBits = static_cast<int>(std::log2(numSets));

    // Draw "TAG" header
    QGraphicsTextItem *tagHeader = cacheScene->addText("TAG");
    tagHeader->setPos(labelWidth + 20, -25);

    // Draw byte offset column headers (in binary)
    for (int byte = 0; byte < blockSize; ++byte) {
        QString bin = QString("%1").arg(byte, offsetBits, 2, QLatin1Char('0'));
        QGraphicsTextItem *txt = cacheScene->addText(bin);
        txt->setScale(0.8);
        txt->setPos(labelWidth + tagWidth + byte * cellWidth + 8, -25);
    }

    // Draw each set with 4 ways (4 rows per set)
    int currentRow = 0;
    for (int set = 0; set < numSets; ++set) {
        for (int way = 0; way < numWays; ++way) {
            // Set and way label in binary
            QString setBin = QString("%1").arg(set, indexBits, 2, QLatin1Char('0'));
            QString wayBin = QString("%1").arg(way, 2, 2, QLatin1Char('0'));
            QGraphicsTextItem *label = cacheScene->addText(setBin + ":" + wayBin);
            label->setPos(0, currentRow * cellHeight + 10);

            // Draw TAG cell
            QRectF tagRect(labelWidth, currentRow * cellHeight, tagWidth, cellHeight);
            cacheScene->addRect(tagRect, pen);

            // Draw each byte cell in this block
            for (int byte = 0; byte < blockSize; ++byte) {
                QRectF cellRect(labelWidth + tagWidth + byte * cellWidth, currentRow * cellHeight, cellWidth, cellHeight);
                cacheScene->addRect(cellRect, pen);
            }
            currentRow++;
        }
    }

    cacheScene->setSceneRect(-10, -20, labelWidth + tagWidth + blockSize * cellWidth + 20, currentRow * cellHeight + 20);
}

void MainWindow::drawFullyAssociative(int cacheSize, int blockSize)
{
    // Fully associative: 1 set, all blocks as rows
    int numBlocks = cacheSize / blockSize;
    int numSets = 1;

    // Initialize cache structure: 1 set, numBlocks ways
    cache.clear();
    cache.resize(numSets);
    cache[0].resize(numBlocks);
    for (int way = 0; way < numBlocks; ++way) {
        cache[0][way].tag = "";
        cache[0][way].data.resize(blockSize);
        cache[0][way].data.fill(0);
        cache[0][way].lineIndex = way;
        cache[0][way].lastaccess = -1;
        cache[0][way].firstaccess = -1;
    }

    if (!cacheScene) {
        cacheScene = new QGraphicsScene(this);
        ui->cacheView->setScene(cacheScene);
    }
    cacheScene->clear();

    const int cellWidth = 40;
    const int cellHeight = 40;
    const int tagWidth = 80;
    const int labelWidth = 80;

    QPen pen(Qt::black);
    pen.setWidth(1);

    // Calculate number of bits needed for byte offset
    int offsetBits = static_cast<int>(std::log2(blockSize));
    int wayBits = static_cast<int>(std::ceil(std::log2(numBlocks)));

    // Draw "TAG" header
    QGraphicsTextItem *tagHeader = cacheScene->addText("TAG");
    tagHeader->setPos(labelWidth + 20, -25);

    // Draw byte offset column headers (in binary)
    for (int byte = 0; byte < blockSize; ++byte) {
        QString bin = QString("%1").arg(byte, offsetBits, 2, QLatin1Char('0'));
        QGraphicsTextItem *txt = cacheScene->addText(bin);
        txt->setScale(0.8);
        txt->setPos(labelWidth + tagWidth + byte * cellWidth + 8, -25);
    }

    // Draw all blocks as rows (all in Set 0)
    for (int block = 0; block < numBlocks; ++block) {
        // Block label: Set 0, Way in binary
        QString wayBin = QString("%1").arg(block, wayBits, 2, QLatin1Char('0'));
        QGraphicsTextItem *label = cacheScene->addText("0:" + wayBin);
        label->setPos(0, block * cellHeight + 10);

        // Draw TAG cell
        QRectF tagRect(labelWidth, block * cellHeight, tagWidth, cellHeight);
        cacheScene->addRect(tagRect, pen);

        // Draw each byte cell in this block
        for (int byte = 0; byte < blockSize; ++byte) {
            QRectF cellRect(labelWidth + tagWidth + byte * cellWidth, block * cellHeight, cellWidth, cellHeight);
            cacheScene->addRect(cellRect, pen);
        }
    }

    cacheScene->setSceneRect(-10, -20, labelWidth + tagWidth + blockSize * cellWidth + 20, numBlocks * cellHeight + 20);
}

void MainWindow::on_nextStep_clicked()
{
    // Get the text from textEdit
    QString allText = ui->textEdit->toPlainText();
    QStringList instructions = allText.split('\n', Qt::SkipEmptyParts);

    // Check if we have more instructions to execute
    if (currentInstructionLine >= instructions.size()) {
        ui->textBrowser->append("\n========================================");
        ui->textBrowser->append("All instructions completed!");
        ui->textBrowser->append("========================================\n");
        return;
    }

    // Get current instruction
    QString instruction = instructions[currentInstructionLine].trimmed();
    currentInstructionLine++;

    ui->textBrowser->append("\n========================================");
    ui->textBrowser->append(QString("INSTRUCTION %1: %2").arg(currentInstructionLine).arg(instruction));
    ui->textBrowser->append("========================================");

    // Parse instruction: "Read Byte 19"
    QStringList parts = instruction.split(' ', Qt::SkipEmptyParts);
    if (parts.size() < 3 || parts[0].toLower() != "read" || parts[1].toLower() != "byte") {
        ui->textBrowser->append(QString("ERROR: Invalid instruction format: %1").arg(instruction));
        ui->textBrowser->append("Expected format: Read Byte <number>");
        return;
    }

    int byteAddress = parts[2].toInt();

    // Step 1: Address Breakdown
    ui->textBrowser->append("\n--- STEP 1: ADDRESS ANALYSIS ---");
    ui->textBrowser->append(QString("Requested byte address: %1 (decimal)").arg(byteAddress));

    // Calculate block address
    int blockAddress = byteAddress / currentBlockSize;
    int byteOffset = byteAddress % currentBlockSize;

    int offsetBits = static_cast<int>(std::log2(currentBlockSize));
    int numSets = cache.size();
    int indexBits = (numSets > 1) ? static_cast<int>(std::log2(numSets)) : 0;

    // Convert byte address to binary
    int totalBits = offsetBits + indexBits + 8; // 8 bits for tag (minimum)
    QString byteAddressBin = QString("%1").arg(byteAddress, totalBits, 2, QLatin1Char('0'));

    ui->textBrowser->append(QString("Binary representation: %1").arg(byteAddressBin));
    ui->textBrowser->append(QString("  - This address tells us which byte in memory we want to access"));

    // Step 2: Calculate which block contains this byte
    ui->textBrowser->append(QString("\n--- STEP 2: BLOCK IDENTIFICATION ---"));
    ui->textBrowser->append(QString("Block size: %1 bytes").arg(currentBlockSize));
    ui->textBrowser->append(QString("Block address calculation: %1 ÷ %2 = %3")
                                .arg(byteAddress).arg(currentBlockSize).arg(blockAddress));
    ui->textBrowser->append(QString("  - Byte %1 is located in Block %2").arg(byteAddress).arg(blockAddress));
    ui->textBrowser->append(QString("  - Block %1 contains bytes %2 through %3")
                                .arg(blockAddress)
                                .arg(blockAddress * currentBlockSize)
                                .arg((blockAddress + 1) * currentBlockSize - 1));

    // Step 3: Calculate set index and tag
    ui->textBrowser->append(QString("\n--- STEP 3: CACHE ADDRESS MAPPING ---"));

    int setIndex = blockAddress % numSets;
    int tag = blockAddress / numSets;

    QString offsetBin = QString("%1").arg(byteOffset, offsetBits, 2, QLatin1Char('0'));
    QString setBin = (indexBits > 0) ? QString("%1").arg(setIndex, indexBits, 2, QLatin1Char('0')) : "";
    QString tagBin = QString("%1").arg(tag, 8, 2, QLatin1Char('0'));

    ui->textBrowser->append(QString("Number of sets in cache: %1").arg(numSets));

    if (numSets > 1) {
        ui->textBrowser->append(QString("Set index calculation: %1 mod %2 = %3")
                                    .arg(blockAddress).arg(numSets).arg(setIndex));
        ui->textBrowser->append(QString("  - Block %1 maps to Set %2 (binary: %3)")
                                    .arg(blockAddress).arg(setIndex).arg(setBin));
    } else {
        ui->textBrowser->append(QString("  - Fully associative cache: only 1 set (Set 0)"));
    }

    ui->textBrowser->append(QString("Tag calculation: %1 ÷ %2 = %3")
                                .arg(blockAddress).arg(numSets).arg(tag));
    ui->textBrowser->append(QString("  - Tag value: %1 (binary: %2)").arg(tag).arg(tagBin));
    ui->textBrowser->append(QString("  - The tag uniquely identifies which block is stored in this set"));

    ui->textBrowser->append(QString("\nByte offset within block: %1 (binary: %2)")
                                .arg(byteOffset).arg(offsetBin));
    ui->textBrowser->append(QString("  - This tells us which specific byte within the block we need"));

    // Address field breakdown
    ui->textBrowser->append(QString("\nAddress field breakdown:"));
    if (indexBits > 0) {
        ui->textBrowser->append(QString("  [Tag: %1 bits | Set Index: %2 bits | Byte Offset: %3 bits]")
                                    .arg(8).arg(indexBits).arg(offsetBits));
        ui->textBrowser->append(QString("  [%1 | %2 | %3]").arg(tagBin).arg(setBin).arg(offsetBin));
    } else {
        ui->textBrowser->append(QString("  [Tag: %1 bits | Byte Offset: %2 bits]")
                                    .arg(8).arg(offsetBits));
        ui->textBrowser->append(QString("  [%1 | %2]").arg(tagBin).arg(offsetBin));
    }

    // Step 4: Cache lookup
    ui->textBrowser->append(QString("\n--- STEP 4: CACHE LOOKUP ---"));
    ui->textBrowser->append(QString("Searching Set %1 for Tag %2...").arg(setIndex).arg(tag));
    ui->textBrowser->append(QString("Set %1 has %2 way(s):").arg(setIndex).arg(cache[setIndex].size()));

    bool hit = false;
    int hitWay = -1;
    QString tagStr = QString::number(tag);

    // Display current state of the set
    for (int way = 0; way < cache[setIndex].size(); ++way) {
        if (cache[setIndex][way].tag.isEmpty()) {
            ui->textBrowser->append(QString("  Way %1: [EMPTY]").arg(way));
        } else {
            ui->textBrowser->append(QString("  Way %1: Tag=%2, First Access=%3, Last Access=%4")
                                        .arg(way)
                                        .arg(cache[setIndex][way].tag)
                                        .arg(cache[setIndex][way].firstaccess)
                                        .arg(cache[setIndex][way].lastaccess));
        }

        if (cache[setIndex][way].tag == tagStr) {
            hit = true;
            hitWay = way;
        }
    }

    // Step 5: Hit or Miss result
    ui->textBrowser->append(QString("\n--- STEP 5: RESULT ---"));

    if (hit) {
        ui->textBrowser->append(QString("✓✓✓ CACHE HIT! ✓✓✓"));
        ui->textBrowser->append(QString("  - Found matching tag %1 in Set %2, Way %3")
                                    .arg(tag).arg(setIndex).arg(hitWay));
        ui->textBrowser->append(QString("  - The requested block is already in the cache!"));
        ui->textBrowser->append(QString("  - We can retrieve byte %1 directly from the cache").arg(byteAddress));
        ui->textBrowser->append(QString("  - Updating last access time from %1 to %2")
                                    .arg(cache[setIndex][hitWay].lastaccess)
                                    .arg(accessCounter));

        cache[setIndex][hitWay].lastaccess = accessCounter;

        // Show the actual byte value
        if (byteOffset < cache[setIndex][hitWay].data.size()) {
            uint8_t value = cache[setIndex][hitWay].data[byteOffset];
            QString hex = QString("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
            ui->textBrowser->append(QString("  - Byte value at offset %1: 0x%2").arg(byteOffset).arg(hex));
        }

    } else {
        ui->textBrowser->append(QString("✗✗✗ CACHE MISS! ✗✗✗"));
        ui->textBrowser->append(QString("  - Tag %1 not found in Set %2").arg(tag).arg(setIndex));
        ui->textBrowser->append(QString("  - The requested block is NOT in the cache"));
        ui->textBrowser->append(QString("  - We must fetch Block %1 from main memory").arg(blockAddress));

        // Step 6: Determine where to place the block
        ui->textBrowser->append(QString("\n--- STEP 6: BLOCK PLACEMENT ---"));

        int targetWay = -1;

        // First, check for empty line
        for (int way = 0; way < cache[setIndex].size(); ++way) {
            if (cache[setIndex][way].tag.isEmpty()) {
                targetWay = way;
                ui->textBrowser->append(QString("  - Found empty Way %1 in Set %2").arg(way).arg(setIndex));
                ui->textBrowser->append(QString("  - No replacement needed, placing block directly"));
                break;
            }
        }

        // If no empty line, use replacement policy
        if (targetWay == -1) {
            ui->textBrowser->append(QString("  - All ways in Set %1 are occupied").arg(setIndex));
            ui->textBrowser->append(QString("  - Must evict a block using replacement policy"));

            QString policyName = (currentReplacementPolicy == 5) ? "LRU (Least Recently Used)" : "FIFO (First In First Out)";
            ui->textBrowser->append(QString("  - Replacement policy: %1").arg(policyName));

            if (currentReplacementPolicy == 5) {
                targetWay = findReplacementWay_LRU(setIndex);
                ui->textBrowser->append(QString("  - LRU selected Way %1 (last accessed at time %2)")
                                            .arg(targetWay)
                                            .arg(cache[setIndex][targetWay].lastaccess));
            } else {
                targetWay = findReplacementWay_FIFO(setIndex);
                ui->textBrowser->append(QString("  - FIFO selected Way %1 (first loaded at time %2)")
                                            .arg(targetWay)
                                            .arg(cache[setIndex][targetWay].firstaccess));
            }

            ui->textBrowser->append(QString("  - Evicting block with Tag %1 from Way %2")
                                        .arg(cache[setIndex][targetWay].tag)
                                        .arg(targetWay));
        }

        // Step 7: Load block from memory
        ui->textBrowser->append(QString("\n--- STEP 7: LOADING FROM MEMORY ---"));
        ui->textBrowser->append(QString("  - Fetching Block %1 from main memory").arg(blockAddress));
        ui->textBrowser->append(QString("  - Loading %1 bytes into Set %2, Way %3")
                                    .arg(currentBlockSize).arg(setIndex).arg(targetWay));

        // Load the entire block from mockData
        int blockStartByte = blockAddress * currentBlockSize;
        cache[setIndex][targetWay].tag = tagStr;
        cache[setIndex][targetWay].firstaccess = accessCounter;
        cache[setIndex][targetWay].lastaccess = accessCounter;

        ui->textBrowser->append(QString("  - Memory addresses being fetched: %1 to %2")
                                    .arg(blockStartByte)
                                    .arg(blockStartByte + currentBlockSize - 1));

        QString blockData = "  - Block data (hex): ";

        // Copy data from mockData (2 hex chars per byte)
        for (int i = 0; i < currentBlockSize; ++i) {
            int memoryByte = blockStartByte + i;
            if (memoryByte * 2 + 1 < 2048) {  // Check bounds
                // Each byte is represented by 2 hex characters
                uint8_t value = 0;
                char highNibble = mockData[memoryByte * 2];
                char lowNibble = mockData[memoryByte * 2 + 1];

                // Convert hex char to value
                auto hexToInt = [](char c) -> uint8_t {
                    if (c >= '0' && c <= '9') return c - '0';
                    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
                    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
                    return 0;
                };

                value = (hexToInt(highNibble) << 4) | hexToInt(lowNibble);
                cache[setIndex][targetWay].data[i] = value;

                QString hex = QString("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
                blockData += hex + " ";
            }
        }

        ui->textBrowser->append(blockData);
        ui->textBrowser->append(QString("  - Successfully loaded Block %1 with Tag %2").arg(blockAddress).arg(tag));
        ui->textBrowser->append(QString("  - Set firstaccess = %1, lastaccess = %1").arg(accessCounter));

        // Show the requested byte value
        if (byteOffset < cache[setIndex][targetWay].data.size()) {
            uint8_t value = cache[setIndex][targetWay].data[byteOffset];
            QString hex = QString("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();
            ui->textBrowser->append(QString("  - Requested byte at offset %1: 0x%2").arg(byteOffset).arg(hex));
        }

        hitWay = targetWay;
    }

    accessCounter++;

    ui->textBrowser->append(QString("\n--- CACHE STATE UPDATED ---"));
    ui->textBrowser->append(QString("Access counter incremented to %1").arg(accessCounter));
    ui->textBrowser->append("Updating visual representation...\n");

    // Redraw the cache to show updated values
    updateCacheVisualization();
}

int MainWindow::findReplacementWay_LRU(int setIndex)
{
    // LRU: Find the way with the smallest lastaccess value
    // This represents the least recently used cache line

    int lruWay = 0;
    int minAccess = cache[setIndex][0].lastaccess;

    for (int way = 1; way < cache[setIndex].size(); ++way) {
        if (cache[setIndex][way].lastaccess < minAccess) {
            minAccess = cache[setIndex][way].lastaccess;
            lruWay = way;
        }
    }

    return lruWay;
}

void MainWindow::updateCacheVisualization()
{
    if (!cacheScene) return;

    const int cellWidth = 40;
    const int cellHeight = 40;
    const int tagWidth = 80;
    const int labelWidth = 80;

    // Clear existing text items (but keep the grid)
    QList<QGraphicsItem*> items = cacheScene->items();
    for (QGraphicsItem* item : items) {
        if (QGraphicsTextItem* textItem = dynamic_cast<QGraphicsTextItem*>(item)) {
            // Don't remove headers (items with y < 0)
            if (textItem->y() >= 0) {
                cacheScene->removeItem(textItem);
                delete textItem;
            }
        }
    }

    // Redraw cache contents based on associativity
    int currentRow = 0;
    for (int set = 0; set < cache.size(); ++set) {
        for (int way = 0; way < cache[set].size(); ++way) {
            // Draw TAG value
            if (!cache[set][way].tag.isEmpty()) {
                QGraphicsTextItem* tagText = cacheScene->addText(cache[set][way].tag);
                tagText->setScale(0.7);
                tagText->setPos(labelWidth + 10, currentRow * cellHeight + 10);
            }

            // Draw data bytes
            for (int byte = 0; byte < cache[set][way].data.size(); ++byte) {
                uint8_t value = cache[set][way].data[byte];
                QString hex = QString("%1").arg(value, 2, 16, QLatin1Char('0')).toUpper();

                QGraphicsTextItem* dataText = cacheScene->addText(hex);
                dataText->setScale(0.8);
                dataText->setPos(labelWidth + tagWidth + byte * cellWidth + 8,
                                 currentRow * cellHeight + 10);
            }

            currentRow++;
        }
    }
}
int MainWindow::findReplacementWay_FIFO(int setIndex)
{
    // FIFO: Find the way with the smallest firstaccess value
    // This represents the oldest cache line (first in)

    int fifoWay = 0;
    int minFirstAccess = cache[setIndex][0].firstaccess;

    for (int way = 1; way < cache[setIndex].size(); ++way) {
        if (cache[setIndex][way].firstaccess < minFirstAccess) {
            minFirstAccess = cache[setIndex][way].firstaccess;
            fifoWay = way;
        }
    }

    return fifoWay;
}
