#include "viewer.h"
#include <QQmlContext>
#include <QDebug>
#include "../DialogPoint/xmlstoryreader.h"
#include <QFile>
#include <QCoreApplication>

Viewer::Viewer(QObject *parent) : QObject(parent)
{

}

void Viewer::initModels(QQmlApplicationEngine &engine)
{

    m_jumpListModel = new JumpListModel();
    m_inventaryModel = new InventaryListModel();

    m_inventaryModel->addItemCopy("Gold", 50);
    openStory();
    //initDialog();
    jumpTo("First");

    QQmlContext *context = engine.rootContext();

    context->setContextProperty("viewer", this);
    context->setContextProperty("jumpListModel", m_jumpListModel);
    context->setContextProperty("inventaryModel", m_inventaryModel);
}

QString Viewer::answer()
{
    return m_answer;
}

/*! Переход в точку  именем \a pointName */
void Viewer::jumpTo(const QString &pointName)
{
    const DialogPoint &point  = m_storyPoints[pointName];

    QStringList availableJumps = filterAvaileble(point.jumps());
    m_jumpListModel->setTexts(availableJumps);

    if (availableJumps.isEmpty())
        m_answer = "GAME OVER!";
    else
        m_answer = point.answer();

    emit answerChanged(m_answer);
}



/*! Метод вызывается, когда пользователь нажимает кнопку на jumpListView
 *  (Может переименовать в PointListVirew ?)
 *  Он получает id точки по её индеексу и обезпечивает переход в данную точку.
 *  Для этого - обновляет значения jumpListModel, добавляет и убирает ресурсы
 *  в инвентаре
*/
void Viewer::onJumpSelected(int index)
{
    QString text = m_jumpListModel->pointText(index); // Спорно. Видимо, придётся перейти к интовым id
    const DialogPoint &point = m_storyPoints[text];

    addToInventary(point.entryAward());
    dropFromInventary(point.entryPayment());

    jumpTo(text);
    m_jumpStack.push_front(text);
    if (m_jumpStack.count() > 5) {
        m_jumpStack.pop_back();
    }
}

/*! Обеспечивает возвращение назад по стеку переходов
 *  при каждом нажатии на кнопку:
 *  - игрок получает назад ресурсы, которые потратил за вход в эту точку
 *  - игрок отдаёт ресурсы, которые получил за вход в эту точку.
 *  - игра переходит к предыдущей точке.
 *
 *  нет необходимости запоминать состояние в каждой точке - достаточно
 *  выполнить действия с ресурсами в обратном порядке. По этому, достигнутое
 *  состояние в стеке не храним, храним только имена точек.
*/
void Viewer::onJumpBack()
{
    if (m_jumpStack.isEmpty())
        return;

    QString text = m_jumpStack.pop();
    const DialogPoint &point = m_storyPoints[text];

    // Изменение инвентаря - вернуть полученное, получить потраченное

    dropFromInventary(point.entryAward());
    addToInventary(point.entryPayment());

    jumpTo(text);

}

/*! Получает на вход список имён точек и возвращает список точек, которые доступны
 *  при текущем уровне ресурсов в инфентаре. Почему этого не делает сама точка?
 *
 *  Так. На сегодняшний момент (06.11.18, утро) filterAvailable может возвращать пустой QStringList
 *  С пустым списком переходов приложение падает
*/
QStringList Viewer::filterAvaileble(const QStringList &jumpsNames) {
    QStringList  availableJumps;

    foreach (QString name, jumpsNames) {
        const DialogPoint &point = m_storyPoints[name]; // Вот тут может всё свалиться, если нет такой точки в m_storyPoints...

        bool hasToPay =
                m_inventaryModel->isInventaryHas(point.entryPayment());
        bool hasKeys =
                m_inventaryModel->isInventaryHas(point.entryKeys());

        if (hasToPay && hasKeys)
            availableJumps.append(point.name());
    }
    return availableJumps;
}


/*! Создаёт набор точек, настраивает их, и добавляет их в m_storyPoints */
void Viewer::initDialog()
{    
    DialogPoint point1;
    DialogPoint point2;
    DialogPoint point3;
    DialogPoint point4;
    DialogPoint point5;

    point1.setAnswer("Your are in first point");
    point1.setName("First point");

    point1.addJump("Second point");
    point1.addJump("Third point");
    point1.addJump("Fourth point");

    point1.addEntryPayment("Gold", 10);
    point1.addEntryKey("Flag", 1);

    point2.setAnswer("Your are in second point");
    point2.setName("Second point");

    point2.addJump("First point");
    point2.addJump("Third point");
    point2.addJump("Fourth point");

    point3.setAnswer("Your are in third point");
    point3.setName("Third point");

    point3.addJump("First point");
    point3.addJump("Second point");
    point3.addJump("Fourth point");
    point3.addEntryAward("Gold", 10);


    point4.setAnswer("Your are in fourth point");
    point4.setName("Fourth point");

    point4.addJump("First point");
    point4.addJump("Second point");
    point4.addJump("Third point");
    point4.addJump("Last point");


    point5.setAnswer("This is last Point! Return to first?");
    point5.setName("Last point");
    point5.addJump("First point");
    point5.addEntryAward("Flag", 1);

    m_storyPoints.insert(point1.name(), point1);
    m_storyPoints.insert(point2.name(), point2);
    m_storyPoints.insert(point3.name(), point3);
    m_storyPoints.insert(point4.name(), point4);
    m_storyPoints.insert(point5.name(), point5);
}

/*! Читает историю из xml-файла и наполняет m_storyPoints */
void Viewer::openStory()
{
    QString path = qApp->applicationDirPath() + "/story.xml";
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qDebug() << "FILE NOT OPEN!!!!!!";
        return;
        // to do: сделать нормальное окно с предкпреждением вместо qDebug
    }
    XmlStoryReader reader(&file);
    while (reader.next()) {
        DialogPoint point = reader.point();
        m_storyPoints.insert(point.name(), point);
    }

}


/*! Добавляет в инвентарь все предметы из словаря  \a items */
void Viewer::addToInventary(const QHash<QString, uint> & items) {
    auto iter = items.begin();
    auto end = items.end();

    while (iter != end) {
        m_inventaryModel->addItemCopy(iter.key(), iter.value());
        iter++;
        qDebug() << "step";
    }
}

/*! Убирает из инвентаря все предметы из словаря  \a items */
void Viewer::dropFromInventary(const QHash<QString, uint> & items) {
    auto iter = items.begin();
    auto end = items.end();

    while (iter != end) {
        m_inventaryModel->dropItemCopy(iter.key(), iter.value()); // Да, это копипаста.
        iter++;
    }
}


