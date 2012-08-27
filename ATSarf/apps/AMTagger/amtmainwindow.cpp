#include <QtGui/QApplication>
#include "amtmainwindow.h"
#include <qjson/parser.h>

#include <QContextMenuEvent>
#include <QTextCodec>
#include <QTextStream>
#include<QDockWidget>
#include<QList>

#include "commonS.h"
#include <addtagview.h>
#include <addtagtypeview.h>
#include <removetagtypeview.h>
#include "global.h"
#include "edittagtypeview.h"

//#include "stemmer.h"
//#include "ATMProgressIFC.h"

#include <QMessageBox>

bool parentCheck;

AMTMainWindow::AMTMainWindow(QWidget *parent) :
    QMainWindow(parent),
    browseFileDlg(NULL)
{
    resize(800,600);

    // Used to check for parent Widget between windows (EditTagTypeView and AMTMainWindow)
    parentCheck = false;

    QDockWidget *dock = new QDockWidget(tr("Text View"), this);
    txtBrwsr = new QTextBrowser(dock);
    dock->setWidget(txtBrwsr);
    txtBrwsr->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(txtBrwsr,SIGNAL(customContextMenuRequested(const QPoint&)), this,SLOT(showContextMenu(const QPoint&)));
    setCentralWidget(txtBrwsr);
    //addDockWidget(Qt::LeftDockWidgetArea, dock);

    createActions();
    createMenus();
    createDockWindows();

    setWindowTitle(tr("Arabic Morphological Tagger"));

    /** clear all views **/
    descBrwsr->clear();
    tagDescription->clear();
    txtBrwsr->clear();
}

void AMTMainWindow::createDockWindows() {

    QDockWidget *dock = new QDockWidget(tr("Tag View"), this);
    dock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    tagDescription = new QTreeWidget(dock);
    tagDescription->setColumnCount(5);
    QStringList columns;
    columns << "Word" << "Tag" << "Source" << "Start" << "Length";
    QTreeWidgetItem* item=new QTreeWidgetItem(columns);
    connect(tagDescription,SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(itemSelectionChanged(QTreeWidgetItem*,int)));
    tagDescription->setHeaderItem(item);

    dock->setWidget(tagDescription);

    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());

    dock = new QDockWidget(tr("Description"), this);
    descBrwsr = new QTreeWidget(dock);
    descBrwsr->setColumnCount(2);
    QStringList columnsD;
    columnsD << "Field" << "Value";
    QTreeWidgetItem* itemD=new QTreeWidgetItem(columnsD);
    descBrwsr->setHeaderItem(itemD);

    dock->setWidget(descBrwsr);
    addDockWidget(Qt::RightDockWidgetArea, dock);
    viewMenu->addAction(dock->toggleViewAction());
}

void AMTMainWindow::showContextMenu(const QPoint &pt) {

    signalMapper = new QSignalMapper(this);
    QMenu * menu = new QMenu();
    QMenu * mTags;
    mTags = menu->addMenu(tr("&Tag"));
    for(int i=0; i<_atagger->tagTypeVector->count(); i++) {
        QAction * taginstance;
        taginstance = new QAction((_atagger->tagTypeVector->at(i)).tag,this);
        signalMapper->setMapping(taginstance, (_atagger->tagTypeVector->at(i)).tag);
        connect(taginstance, SIGNAL(triggered()), signalMapper, SLOT(map()));
        mTags->addAction(taginstance);
    }
    connect(signalMapper, SIGNAL(mapped(const QString &)), this, SLOT(tag(QString)));
    menu->addAction(untagMAct);
    menu->addSeparator();
    menu->addAction(addtagAct);
    if(txtBrwsr->textCursor().selectedText().isEmpty()) {
        mTags->setEnabled(false);
        untagMAct->setEnabled(false);
    }
    else {
        untagMAct->setEnabled(true);
    }
    menu->exec(txtBrwsr->mapToGlobal(pt));
    delete menu;
}

/*
void AMTMainWindow::contextMenuEvent(QContextMenuEvent *event)
 {
     QMenu * menu;
     QMenu * tags;
     menu = new QMenu();
     tags = menu->addMenu(tr("&Tag"));
     for(int i=0; i<_atagger->tagTypeVector->count(); i++) {        
         QAction * taginstance;
         char * tagValue = (_atagger->tagTypeVector->at(i)).tag.toLocal8Bit().data();
         taginstance = new QAction(tr(tagValue), this);
         connect(taginstance, SIGNAL(triggered()), this, SLOT(tag(tagValue)));
         tags->addAction(taginstance);
     }
     menu->addAction(untagAct);
     menu->addAction(addtagAct);
     menu->exec(event->globalPos());
}
*/

void AMTMainWindow::open() {
    _atagger = NULL;
    _atagger = new ATagger();
    if (browseFileDlg == NULL) {
        QString dir = QDir::currentPath();
        browseFileDlg = new QFileDialog(NULL, QString("Open File"), dir, QString("All Files (*)"));
        browseFileDlg->setOptions(QFileDialog::DontUseNativeDialog);
        browseFileDlg->setFileMode(QFileDialog::ExistingFile);
        browseFileDlg->setViewMode(QFileDialog::Detail);
    }
     if(browseFileDlg->exec()) {
        QStringList files = browseFileDlg->selectedFiles();
        QString fileName = files[0];

        QFile Ifile(fileName);
        if (!Ifile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::about(this, tr("Input File"),
                         tr("The <b>File</b> can't be opened!"));
        }

        QString text = Ifile.readAll();
        Ifile.close();

        _atagger->tagFile = fileName.replace(QString(".txt"), ".tags");
        QFile ITfile(_atagger->tagFile);
        if (!ITfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
            QMessageBox::about(this, tr("Input Tag File"),
                         tr("The <b>Tag File</b> can't be opened!"));
        }

        QByteArray Tags = ITfile.readAll();

        ITfile.close();

        startTaggingText(text);
        process(Tags);
        finishTaggingText();
    }
}

void AMTMainWindow::startTaggingText(QString & text) {
    if (this==NULL)
        return;
    QTextBrowser * taggedBox=txtBrwsr;
    //QTextEdit * taggedBox = txtBrwsr;
    taggedBox->clear();
    taggedBox->setLayoutDirection(Qt::RightToLeft);
    QTextCursor c=taggedBox->textCursor();
    c.clearSelection();
    c.movePosition(QTextCursor::Start,QTextCursor::MoveAnchor);
    taggedBox->setTextCursor(c);
    taggedBox->setTextBackgroundColor(Qt::white);
    taggedBox->setTextColor(Qt::black);
    taggedBox->setText(text);
}

void AMTMainWindow::process(QByteArray & json) {

    QJson::Parser parser;
    bool ok;

    QVariantMap result = parser.parse (json,&ok).toMap();

    if (!ok) {
        QMessageBox::about(this, tr("Input Tag File"),
                     tr("The <b>Tag File</b> has a wrong format"));
    }

    _atagger->textFile = result["file"].toString();
    _atagger->tagtypeFile = result["TagTypeFile"].toString();

    foreach(QVariant tag, result["TagArray"].toList()) {

        QVariantMap tagElements = tag.toMap();
        int start = tagElements["pos"].toInt();
        int length = tagElements["length"].toInt();
        QString tagtype = tagElements["type"].toString();
        QString source = tagElements["source"].toString();
        bool check;
        if(source == "sarf") {
            check = _atagger->insertTag(tagtype,start,length,sarf);
        }
        else {
            check = _atagger->insertTag(tagtype,start,length,user);
        }
    }

    /** Read the TagType file and store it **/

    QFile ITfile(_atagger->tagtypeFile);
    if (!ITfile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::about(this, tr("Input Tag File"),
                     tr("The <b>Tag Type File</b> can't be opened!"));
    }

    QByteArray tagtypedata = ITfile.readAll();
    ITfile.close();

    result = parser.parse (tagtypedata,&ok).toMap();

    if (!ok) {
        QMessageBox::about(this, tr("Input Tag File"),
                     tr("The <b>Tag File</b> has a wrong format"));
    }

    foreach(QVariant type, result["TagSet"].toList()) {
        QString tag;
        QString desc;
        int id;
        QString foreground_color;
        QString background_color;
        int font;
        bool underline;
        bool bold;
        bool italic;

        QVariantMap typeElements = type.toMap();

        tag = typeElements["Tag"].toString();
        desc = typeElements["Description"].toString();
        id = typeElements["Legend"].toInt();
        foreground_color = typeElements["foreground_color"].toString();
        background_color = typeElements["background_color"].toString();
        font = typeElements["font"].toInt();
        underline = typeElements["underline"].toBool();
        bold = typeElements["bold"].toBool();
        italic = typeElements["italic"].toBool();
        _atagger->insertTagType(tag,desc,id,foreground_color,background_color,font,underline,bold,italic);
    }

    /** Apply Tags on Input Text **/

    for(int i =0; i< _atagger->tagVector->count(); i++) {
        for(int j=0; j< _atagger->tagTypeVector->count(); j++) {
            if((_atagger->tagVector->at(i)).type == (_atagger->tagTypeVector->at(j)).tag) {
                int start = (_atagger->tagVector->at(i)).pos;
                int length = (_atagger->tagVector->at(i)).length;
                QColor bgcolor((_atagger->tagTypeVector->at(j)).bgcolor);
                QColor fgcolor((_atagger->tagTypeVector->at(j)).fgcolor);
                int font = (_atagger->tagTypeVector->at(j)).font;
                bool underline = (_atagger->tagTypeVector->at(j)).underline;
                bool bold = (_atagger->tagTypeVector->at(j)).bold;
                bool italic = (_atagger->tagTypeVector->at(j)).italic;
                tagWord(start,length,fgcolor,bgcolor,font,underline,italic,bold);
                break;
            }
        }
    }

    /** Add Tags to tagDescription Tree **/

    fillTreeWidget();
    createTagMenu();
}

void AMTMainWindow::tagWord(int start, int length, QColor fcolor, QColor  bcolor,int font, bool underline, bool italic, bool bold){
    if (this==NULL)
        return;
    QTextBrowser * taggedBox= txtBrwsr;
    //QTextEdit * taggedBox = txtBrwsr;
    QTextCursor c=taggedBox->textCursor();
#if 0
    int lastpos=c.position();
    int diff=start-lastpos;
    if (diff>=0)
        c.movePosition(QTextCursor::Right,QTextCursor::MoveAnchor,diff);
    else
        c.movePosition(QTextCursor::Left,QTextCursor::MoveAnchor,-diff);
    c.movePosition(QTextCursor::Right,QTextCursor::KeepAnchor,length);
#else
    /*if (length>200) {
        start=start+length-1;
        length=5;
        color=Qt::red;
    }*/
    c.setPosition(start,QTextCursor::MoveAnchor);
    c.setPosition(start+length,QTextCursor::KeepAnchor);
#endif
    taggedBox->setTextCursor(c);
    taggedBox->setTextColor(fcolor);
    taggedBox->setTextBackgroundColor(bcolor);
    taggedBox->setFontUnderline(underline);
    taggedBox->setFontItalic(italic);
    if(bold)
        taggedBox->setFontWeight(QFont::Bold);
    else
        taggedBox->setFontWeight(QFont::Normal);
    taggedBox->setFontPointSize(font);
    //taggedBox->setFontPointSize();
}

void AMTMainWindow::finishTaggingText() {
    if (this==NULL)
        return;
    QTextBrowser * taggedBox= txtBrwsr;
    //QTextEdit * taggedBox = txtBrwsr;
    QTextCursor c=taggedBox->textCursor();
    c.movePosition(QTextCursor::End,QTextCursor::MoveAnchor);
    taggedBox->setTextCursor(c);
}

bool AMTMainWindow::saveFile(const QString &fileName, QByteArray &tagD, QByteArray &tagTD) {

    QFile tfile(fileName);
    QString tfileName = fileName;
    tfileName.replace(QString(".tags"), QString(".tagtypes"));
    QFile ttfile(tfileName);
    if (!tfile.open(QFile::WriteOnly | QFile::Text)) {
        /*QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(fileName)
                             .arg(tfile.errorString()));*/
        return false;
    }

    if (!ttfile.open(QFile::WriteOnly | QFile::Text)) {/*
        QMessageBox::warning(this, tr("Application"),
                             tr("Cannot write file %1:\n%2.")
                             .arg(tfileName)
                             .arg(ttfile.errorString()));*/
        return false;
    }

    QTextStream outt(&tfile);
    outt << tagD;

    QTextStream outtt(&ttfile);
    outtt << tagTD;

    return true;
 }

void AMTMainWindow::save() {

    /** Save to Default Destination **/

    QByteArray tagData = _atagger->dataInJsonFormat(tagV);
    QByteArray tagtypeData = _atagger->dataInJsonFormat(tagTV);

    saveFile(_atagger->tagFile,tagData,tagtypeData);
}

bool AMTMainWindow::saveas() {

    /** Save to specified Destination with .tag extension **/

    QByteArray tagData = _atagger->dataInJsonFormat(tagV);
    QByteArray tagtypeData = _atagger->dataInJsonFormat(tagTV);

    QString fileName = QFileDialog::getSaveFileName(this);
         if (fileName.isEmpty())
             return false;

         return saveFile(fileName,tagData,tagtypeData);
}
/** Not needed Anymore**/
void AMTMainWindow::tagadd() {
    //QTextCursor cursor = txtBrwsr->textCursor();
    //AddTagView * atv = new AddTagView(cursor.selectionStart(), cursor.selectionEnd(), this);
    if(txtBrwsr->textCursor().selectedText() != "") {
        AddTagView * atv = new AddTagView(txtBrwsr, this);
        atv->show();
    }
    else {
        switch( QMessageBox::information( this, "Add Tag","No word is selected for tagging!","&Ok",0,0) ) {
            return;
        }
    }
}

void AMTMainWindow::tagremove() {
    QTextCursor cursor = txtBrwsr->textCursor();
    int start = cursor.selectionStart();
    int length = cursor.selectionEnd() - cursor.selectionStart();
    for(int i=0; i < _atagger->tagVector->count(); i++) {
        if((_atagger->tagVector->at(i)).pos == start) {
            tagWord(start,length,QColor("black"),QColor("white"),9,false,false,false);
            _atagger->tagVector->remove(i);
            cursor.clearSelection();
            fillTreeWidget();
        }
    }
}

void AMTMainWindow::edittagtypes() {
    EditTagTypeView * ettv = new EditTagTypeView(this);
    ettv->show();
}

void AMTMainWindow::tagtypeadd() {
    parentCheck = false;
    AddTagTypeView * attv = new AddTagTypeView(this);
    attv->show();
}

void AMTMainWindow::tagtyperemove() {
    RemoveTagTypeView * rttv = new RemoveTagTypeView(txtBrwsr,tagDescription,this);
    rttv->show();
}

void AMTMainWindow::tag(QString tagValue) {
    if(txtBrwsr->textCursor().selectedText() != "") {
        QTextCursor cursor = txtBrwsr->textCursor();
        int start = cursor.selectionStart();
        int length = cursor.selectionEnd() - cursor.selectionStart();
        _atagger->insertTag(tagValue, start, length, user);

        for(int i=0; i< _atagger->tagTypeVector->count(); i++) {
            if((_atagger->tagTypeVector->at(i)).tag == tagValue) {
                QColor bgcolor((_atagger->tagTypeVector->at(i)).bgcolor);
                QColor fgcolor((_atagger->tagTypeVector->at(i)).fgcolor);
                int font = (_atagger->tagTypeVector->at(i)).font;
                bool underline = (_atagger->tagTypeVector->at(i)).underline;
                bool bold = (_atagger->tagTypeVector->at(i)).bold;
                bool italic = (_atagger->tagTypeVector->at(i)).italic;

                tagWord(cursor.selectionStart(),cursor.selectionEnd()-cursor.selectionStart(),fgcolor,bgcolor,font,underline,italic,bold);
            }
        }
        cursor.clearSelection();
        fillTreeWidget();
    }
    else {
        switch( QMessageBox::information( this, "Add Tag","No word is selected for tagging!","&Ok",0,0) ) {
            return;
        }
    }
}

void AMTMainWindow::untag() {

    QTextCursor cursor = txtBrwsr->textCursor();
    int start = cursor.selectionStart();
    int length = cursor.selectionEnd() - cursor.selectionStart();
    for(int i=0; i < _atagger->tagVector->count(); i++) {
        if((_atagger->tagVector->at(i)).pos == start) {
            tagWord(start,length,QColor("black"),QColor("white"),12,false,false,false);
            _atagger->tagVector->remove(i);
            cursor.clearSelection();
            fillTreeWidget();
        }
    }
}

void AMTMainWindow::addtagtype() {
    AddTagTypeView * attv = new AddTagTypeView();
    attv->show();

}

void AMTMainWindow::about() {

}

void AMTMainWindow::aboutQt() {

}

void AMTMainWindow::createActions()
{
    openAct = new QAction(tr("&Open..."), this);
    openAct->setShortcuts(QKeySequence::Open);
    openAct->setStatusTip(tr("Open an existing file"));
    connect(openAct, SIGNAL(triggered()), this, SLOT(open()));

    saveAct = new QAction(tr("&Save"), this);
    saveAct->setShortcuts(QKeySequence::Save);
    saveAct->setStatusTip(tr("Save the document to disk"));
    connect(saveAct, SIGNAL(triggered()), this, SLOT(save()));

    saveasAct = new QAction(tr("&SaveAs"), this);
    saveasAct->setShortcuts(QKeySequence::SaveAs);
    saveasAct->setStatusTip(tr("Print the document"));
    connect(saveasAct, SIGNAL(triggered()), this, SLOT(saveas()));

    exitAct = new QAction(tr("E&xit"), this);
    exitAct->setShortcuts(QKeySequence::Quit);
    exitAct->setStatusTip(tr("Exit the application"));
    connect(exitAct, SIGNAL(triggered()), this, SLOT(close()));

    tagaddAct = new QAction(tr("&Tag"), this);
    //tagaddAct->setShortcuts(QKeySequence::);
    tagaddAct->setStatusTip(tr("Add a Tag"));
    connect(tagaddAct, SIGNAL(triggered()), this, SLOT(tagadd()));

    tagremoveAct = new QAction(tr("&Untag"), this);
    tagremoveAct->setStatusTip(tr("Remove a Tag"));
    connect(tagremoveAct, SIGNAL(triggered()), this, SLOT(tagremove()));

    edittagtypesAct = new QAction(tr("EditTagTypes..."),this);
    edittagtypesAct->setStatusTip(tr("Edit Tag Types"));
    connect(edittagtypesAct, SIGNAL(triggered()), this, SLOT(edittagtypes()));

    tagtypeaddAct = new QAction(tr("&TagType Add"), this);
    tagtypeaddAct->setStatusTip(tr("Add a TagType"));
    connect(tagtypeaddAct, SIGNAL(triggered()), this, SLOT(tagtypeadd()));

    tagtyperemoveAct = new QAction(tr("&TagType Remove"), this);
    tagtyperemoveAct->setStatusTip(tr("Remove a TagType"));
    connect(tagtyperemoveAct, SIGNAL(triggered()), this, SLOT(tagtyperemove()));

    //tagAct = new QAction(tr("Tag"), this);
    //tagAct->setShortcuts(QKeySequence::Cut);
    //tagAct->setStatusTip(tr("Tag selected word"));
    //connect(tagAct, SIGNAL(triggered()), this, SLOT(tag(QString)));

    untagMAct = new QAction(tr("Untag"), this);
    //untagAct->setShortcuts(QKeySequence::Copy);
    untagMAct->setStatusTip(tr("Untag selected word"));
    connect(untagMAct, SIGNAL(triggered()), this, SLOT(untag()));

    addtagAct = new QAction(tr("&Add TagType"), this);
    //addtagAct->setShortcuts(QKeySequence::Paste);
    addtagAct->setStatusTip(tr("Add a TagType"));
    connect(addtagAct, SIGNAL(triggered()), this, SLOT(addtagtype()));

    aboutAct = new QAction(tr("&About"), this);
    aboutAct->setStatusTip(tr("Show the application's About box"));
    connect(aboutAct, SIGNAL(triggered()), this, SLOT(about()));

    aboutQtAct = new QAction(tr("About &Qt"), this);
    aboutQtAct->setStatusTip(tr("Show the Qt library's About box"));
    connect(aboutQtAct, SIGNAL(triggered()), qApp, SLOT(aboutQt()));
    connect(aboutQtAct, SIGNAL(triggered()), this, SLOT(aboutQt()));
}

void AMTMainWindow::createMenus()
{
    fileMenu = menuBar()->addMenu(tr("&File"));
    fileMenu->addAction(openAct);
    fileMenu->addAction(saveAct);
    fileMenu->addAction(saveasAct);
    fileMenu->addSeparator();
    fileMenu->addAction(exitAct);

    tagMenu = menuBar()->addMenu(tr("&Tags"));
    mTags = tagMenu->addMenu(tr("&Tag"));
    createTagMenu();

    tagMenu->addAction(tagremoveAct);
    tagMenu->addSeparator();
    tagMenu->addAction(edittagtypesAct);

    /*
    tagtypeMenu = menuBar()->addMenu(tr("&TagType"));
    tagtypeMenu->addAction(tagtypeaddAct);
    tagtypeMenu->addAction(tagtyperemoveAct);
    tagtypeMenu->addSeparator();
    */

    viewMenu = menuBar()->addMenu(tr("&View"));

    helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(aboutAct);
    helpMenu->addAction(aboutQtAct);
}

void AMTMainWindow::createTagMenu() {
    mTags->clear();
    signalMapperM = new QSignalMapper(this);
    for(int i=0; i<_atagger->tagTypeVector->count(); i++) {
        QAction * taginstance;
        taginstance = new QAction((_atagger->tagTypeVector->at(i)).tag,this);
        signalMapperM->setMapping(taginstance, (_atagger->tagTypeVector->at(i)).tag);
        connect(taginstance, SIGNAL(triggered()), signalMapperM, SLOT(map()));
        mTags->addAction(taginstance);
    }
    connect(signalMapperM, SIGNAL(mapped(const QString &)), this, SLOT(tag(QString)));
}

void AMTMainWindow::fillTreeWidget() {
    tagDescription->clear();
     QList<QTreeWidgetItem *> items;
     for(int i=0; i < _atagger->tagVector->count(); i++) {
         QStringList entry;
         QTextCursor cursor = txtBrwsr->textCursor();
         int pos = (_atagger->tagVector->at(i)).pos;
         int length = (_atagger->tagVector->at(i)).length;
         cursor.setPosition(pos,QTextCursor::MoveAnchor);
         cursor.setPosition(pos + length,QTextCursor::KeepAnchor);
         QString text = cursor.selectedText();
         entry<<text;
         entry<<(_atagger->tagVector->at(i)).type;
         QString src;
         if((_atagger->tagVector->at(i)).source == user) {
            src = "user";
        }
        else {
            src = "sarf";
        }
        entry<<src;
        entry<<QString::number(pos);
        entry<<QString::number(length);
        items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
    }
     tagDescription->insertTopLevelItems(0, items);
}

void AMTMainWindow::itemSelectionChanged(QTreeWidgetItem* item ,int i) {
    descBrwsr->clear();
    txtBrwsr->textCursor().clearSelection();
    QList<QTreeWidgetItem *> items;
    QString type = item->text(1);
    for(int j=0; j < _atagger->tagTypeVector->count(); j++) {
        if((_atagger->tagTypeVector->at(j)).tag == type) {
            QString desc = (_atagger->tagTypeVector->at(j)).description;
            QColor bgcolor((_atagger->tagTypeVector->at(j)).bgcolor);
            QColor fgcolor((_atagger->tagTypeVector->at(j)).fgcolor);
            int font = (_atagger->tagTypeVector->at(j)).font;
            bool underline = (_atagger->tagTypeVector->at(j)).underline;
            bool bold = (_atagger->tagTypeVector->at(j)).bold;
            bool italic = (_atagger->tagTypeVector->at(j)).italic;

            QTextCursor c = txtBrwsr->textCursor();
            c.setPosition(item->text(3).toInt(),QTextCursor::MoveAnchor);
            c.setPosition(item->text(3).toInt() + item->text(4).toInt(),QTextCursor::KeepAnchor);
            txtBrwsr->setTextCursor(c);

            QStringList entry;
            entry << "Word" << item->text(0);
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Description" <<desc;
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "TagType" << type;
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Source" << item->text(2);
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Position" << item->text(3);
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Length" << item->text(4);
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Background Color" << bgcolor.name();
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Foreground Color" << fgcolor.name();
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            entry << "Font Size" << QString::number(font);
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            if(underline)
                entry << "Underline" << "True";
            else
                entry << "Underline" <<  "False";
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            if(bold)
                entry << "Bold" << "True";
            else
                entry << "Bold" << "False";
            items.append(new QTreeWidgetItem((QTreeWidget*)0, entry));
            entry.clear();
            if(italic)
                entry << "Italic" << "True";
            else
                entry << "Italic" << "False";

            descBrwsr->insertTopLevelItems(0, items);
            break;
        }
    }
}

void AMTMainWindow::sarfTagging() {
    /*
    QFile Ofile("output.txt");
    QFile Efile("error.txt");
    Ofile.open(QIODevice::WriteOnly);
    Efile.open(QIODevice::WriteOnly);
    EmptyProgressIFC * pIFC = new EmptyProgressIFC();

    Sarf srf;
    bool all_set = srf.start(&Ofile,&Efile, pIFC);

    Sarf::use(&srf);

    if(!all_set) {
        //error<<"Can't Set up Project";
    }
    else {
        //cout<<"All Set"<<endl;
    }

    char filename[100];
    //cout << "please enter a file name: " << endl;
    //cin >> filename;

    QFile Ifile(filename);
    if (!Ifile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        //cerr << "error opening file." << endl;
        //return -1;
    }

    QTextStream in(&Ifile);
    while (!in.atEnd()) {
        QString line = in.readLine();
        //run_process(line);
    }

    srf.exit();
    */
}

AMTMainWindow::~AMTMainWindow() {

}

int main(int argc, char *argv[])
{
    QTextCodec::setCodecForTr( QTextCodec::codecForName( "UTF-8" ) );
    QTextCodec::setCodecForCStrings( QTextCodec::codecForName( "UTF-8" ) );

    _atagger=new ATagger();

    //Stemmer stemmer = new Stemmer("Ameen",0);
    //stemmer();

    QApplication a(argc, argv);
    AMTMainWindow w;
    w.show();

    return a.exec();
}
