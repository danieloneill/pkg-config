#include <QtCore>
#include <QCoreApplication>
#include <QSettings>
#include <QRegExp>

int main(int argc, char **argv);

class Pkg;
class PkgConfig : public QCoreApplication
{
Q_OBJECT
	QList<Pkg *>	packages;
	QStringList	paths;
	QStringList	args;

	bool showErrors, showWarnings;

public:
	QSettings *settings;

	PkgConfig(int argc, char **argv);
	void showHelp();
	void pathAdd(QString);
	void pathRemove(QString);
	void pathList();
	void pathWrite();

	int commandLine();
	bool load();

	void warning(QString err);
	void error(QString err);
	Pkg *getPackage( QString name, QString verOp="", QString version="" );
	bool addPackage( QFileInfo fileInfo, QHash<QString, QString> attributes, QHash<QString, QString> definitions );
	bool parseContent( QFileInfo fileInfo, QString content );
	bool addPCFile(QFileInfo fileInfo);
	bool loadPCFiles();
	bool scanPath(QString path, int recurse);
	bool scanPaths();
	bool readPaths();
	bool resolvePackages();
};

class Pkg : public QObject
{
Q_OBJECT
protected:
	bool resolving;
	PkgConfig *manager;

public:
	Pkg(PkgConfig *p) : QObject(p) { manager = p; };

	QString	name;
	QFileInfo	fileInfo;

	QList<Pkg *>	conflicts;
	QList<Pkg *>	depends;

	QHash<QString, QString>	attributes;
	QHash<QString, QString>	definitions;

	bool resolvePackageConflicts();
	bool resolvePackageRequires();
	QList<Pkg *> resolvePackageAttributeEntries(QStringList keys);
	QHash<Pkg *, Pkg *> gatherConflicts();
	void printConflicts();
	QStringList getLibs();
	QStringList getCFlags();
};

