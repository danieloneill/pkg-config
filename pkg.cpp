#include "pkg.h"

#define VERSION "1.00"

int main(int argc, char **argv)
{
	PkgConfig pkg(argc, argv);

	return pkg.commandLine();
}

PkgConfig::PkgConfig(int argc, char **argv) : QCoreApplication(argc, argv)
{
	settings = new QSettings("freedesktop", "pkg-config", this);

	showWarnings = showErrors = false;

	args = arguments();
}

int PkgConfig::commandLine()
{
	QStringList actions;
	QStringList packs;

	int x;
	//fprintf(stderr, "%s\n", args.join(" ").toStdString().c_str() );
	for( x=1; x < args.size(); x++ )
	{
		if( args.at(x).left(1) != "-" )
		{
			// FIXME handle versioning on the command line
			packs << args.at(x);
		} else if( args.at(x) == "--atleast-pkgconfig-version" )
		{
			x++;
			if( VERSION >= args.at(x) )
				return 0;
			return -1;
		} else if( args.at(x) == "--print-warnings" )
		{
			showErrors = true;
			showWarnings = true;
		} else if( args.at(x) == "--print-errors" )
		{
			showErrors = true;
		} else if( args.at(x) == "--short-errors" )
		{
			fprintf(stderr, "pkg-config --short-errors: This doesn't do anything yet.\n");
			showErrors = true;
		} else if( args.at(x) == "--exists" )
		{
			actions << "exists";
		} else if( args.at(x) == "--version" )
		{
			printf(VERSION "\n");
			return 0;
		} else if( args.at(x) == "-h" )
		{
			showHelp();
			return 0;
		} else if( args.at(x) == "-A" )
		{
			x++;
			QString path = args.at(x);
			pathAdd(path);
		} else if( args.at(x) == "-D" )
		{
			x++;
			QString path = args.at(x);
			pathRemove(path);
		} else if( args.at(x) == "-L" )
		{
			x++;
			pathList();
			return 0;
		} else if( args.at(x) == "--libs" )
		{
			actions << "libs";
		} else if( args.at(x) == "--cflags" )
		{
			actions << "cflags";
		} else {
			fprintf(stderr, "Unrecognized option: %s\n", args.at(x).toStdString().c_str());
		}
	}

	if( actions.size() == 0 || packs.size() == 0 )
		return -1;

	load();

	bool wroteOut = false;
	int ret = 0;

	QString action, pack;
	foreach( action, actions )
	{
		foreach( pack, packs )
		{
			Pkg *pkg = getPackage(pack);
			if( action == "exists" )
			{
				if( !pkg ) ret = 1;
			}

			if( !pkg )
				continue;

			pkg->resolvePackageRequires();
			pkg->resolvePackageConflicts();

			if( action == "cflags" )
			{
				QStringList flags = pkg->getCFlags();
				printf( " %s", flags.join(" ").toStdString().c_str() );
				wroteOut = true;
			}
			if( action == "libs" )
			{
				QStringList flags = pkg->getLibs();
				printf( " %s", flags.join(" ").toStdString().c_str() );
				wroteOut = true;
			}
		}
	}

	if( wroteOut )
		printf("\n");

	return ret;
}

void PkgConfig::pathList()
{
	readPaths();
	QString path;
	foreach( path, paths )
	{
		printf("%s\n", path.toStdString().c_str());
	}
}

void PkgConfig::pathRemove(QString path)
{
	readPaths();
	paths.removeOne(path);
	pathWrite();
}

void PkgConfig::pathAdd(QString path)
{
	readPaths();
	paths.append(path);
	pathWrite();
}

void PkgConfig::pathWrite()
{
	settings->beginWriteArray("paths");
	for( int i=0; i < paths.size(); i++ ) {
		settings->setArrayIndex(i);
		settings->setValue("path", paths.at(i));
	}
	settings->endArray();
	settings->sync();
}

void PkgConfig::showHelp()
{
	printf("pkg-config [-A <package config directory>]\n");
	printf("           [-D <package config directory>]\n");
	printf("           [-L]\n");
	printf("           [--atleast-pkgconfig-version]\n");
	printf("           [--version]\n");
	printf("           [--exists]\n");
	printf("           [--short-errors]\n");
	printf("           [--print-warnings]\n");
	printf("           [--print-errors]\n");
	printf("           [--cflags <package name>]\n");
	printf("           [--libs <package name>]\n");
}

bool PkgConfig::load()
{
	if( !readPaths() )
	{
		error("No directories are configured to scan for .pc files.");
		return false;
	}

	if( !scanPaths() )
	{
		//error("Couldn't find any .pc files in one or more of the configured paths.");
	}

	if( !loadPCFiles() )
	{
		error("Failed to load and parse one or more .pc files.");
	}

	return true;
}

void PkgConfig::warning(QString err)
{
	if( showWarnings )
		fprintf( stderr, "Warning: %s\n", err.toStdString().c_str() );
}

void PkgConfig::error(QString err)
{
	if( showWarnings )
		fprintf( stderr, "Error: %s\n", err.toStdString().c_str() );
}

bool Pkg::resolvePackageConflicts()
{
	QStringList keys;
	keys << "Conflicts" << "Conflicts.private";
	conflicts = resolvePackageAttributeEntries(keys);
	if( conflicts.size() > 0 )
		return true;
	return false;
}

bool Pkg::resolvePackageRequires()
{
	QStringList keys;
	keys << "Requires" << "Requires.private";
	depends = resolvePackageAttributeEntries(keys);
	if( depends.size() > 0 )
		return true;
	return false;
}

QList<Pkg *> Pkg::resolvePackageAttributeEntries(QStringList keys)
{
	QList<Pkg *> entries;
	QString tmp;
	QRegExp oppairre("\\s+");

	QString attrlist;

	resolving = true;

	QStringList tmplist;
	foreach( tmp, keys )
	{
		tmplist.append( attributes.value(tmp).split(QRegExp("\\s+")) );
	}
	attrlist = tmplist.join(" ");

	// it's empty, just whitespace.
	if( oppairre.exactMatch(attrlist) )
	{
		resolving = false;
		return entries;
	}


	QRegExp re("(\\S+ [\\s+[\\>\\=|\\<\\=|\\<|\\>|\\=]\\s+\\d+]?)");
	QStringList deplist = attrlist.split(re, QString::SkipEmptyParts);
	if( deplist.size() == 0 && attrlist.size() > 0 )
	{
		manager->warning(QString("Couldn't parse requirements of '%1': %2").arg(name).arg(attrlist));
		resolving = false;
		return entries;
	}

	int cntA = deplist.count();
	int x;

	for( x=0; x < cntA; x++ )
	{
		tmp = deplist.at(x);
		QStringList parts = tmp.split(oppairre, QString::SkipEmptyParts);
		if( parts.size() == 0 )
		{
			manager->warning(QString("Couldn't decipher dependency entry '%1' in package '%2'").arg(tmp).arg(name));
			continue;
		}

		QString dpack, op, ver;

		dpack = parts.at(0);
		if( parts.size() == 3 )
		{
			op = parts.at(1);
			ver = parts.at(2);
		}

		Pkg *dep = manager->getPackage( dpack, op, ver );

		if( dep )
		{
//			printf("%s\n", (QString("Adding dependency '%1' for package '%2'").arg(dpack).arg(name)).toStdString().c_str());
			if( dep->resolving )
			{
				manager->warning("Not following dependency calculation, this one is cyclic.");
				continue;
			}

			if( !entries.contains(dep) )
			{
				entries.append( dep );
				entries.append( dep->resolvePackageAttributeEntries(keys) );
			}
		} else {
			manager->error(QString("Failed to find dependency '%1' for package '%2'").arg(dpack).arg(name));
		}
	}

	resolving = false;

	return entries;
}

Pkg *PkgConfig::getPackage( QString name, QString verOp, QString version )
{
	Pkg *cur, *ret = NULL;
	foreach( cur, packages )
	{
		if( cur->name == name )
		{
			if( !verOp.isEmpty() )
			{
				bool ret;
				QString curversion = cur->attributes.value("Version");
				if( verOp == "=" )
					ret = (curversion == version);
				else if( verOp == "<=" )
					ret = (curversion <= version);
				else if( verOp == ">=" )
					ret = (curversion >= version);
				else if( verOp == "<" )
					ret = (curversion < version);
				else if( verOp == ">" )
					ret = (curversion > version);
				else {
					warning(QString("Sorry, I don't know how to handle operator '%1' in file '%2'").arg(verOp).arg(cur->fileInfo.filePath()));
					continue;
				}

				if( !ret )
					continue;
			}

			if( ret )
			{
				if( ret->attributes.value("Version") < cur->attributes.value("Version") )
					ret = cur;
			} else
				ret = cur;
		}
	}

	if( ret )
		return ret;

	warning(QString("Couldn't find a package named '%1'").arg(name));
	return NULL;
}

QHash<Pkg *, Pkg *> Pkg::gatherConflicts()
{
	QHash<Pkg *, Pkg *> pairs;

	Pkg *cur;

	foreach( cur, conflicts )
	{
		// conflict is already flagged in the other direction
		// eg, cur has this node marked as in conflict with it
		// or, another dependency path has already mapped it.
		if( !cur->conflicts.contains( this )
				&& !conflicts.contains( cur ) )
		{
			pairs.insert( this, cur );

			pairs = pairs.unite( cur->gatherConflicts() );
		}
	}

	return pairs;
}

void Pkg::printConflicts()
{
	QHash<Pkg *, Pkg *> table = gatherConflicts();

	Pkg *cur;
	foreach( cur, table.keys() )
	{
		Pkg *other = table.value(cur);
		QString verA = cur->name;
		QString verB = other->name;

		manager->error(QString("Package %1 and package %2 are conflicting").arg(verA).arg(verB));
	}
}

QStringList Pkg::getLibs()
{
	QStringList ret;
	QRegExp rx("\\$\\{(\\w+)\\}");
	QString flags, def;
	int reps = -1;

	flags = attributes.value("Libs");
	do {
		reps = rx.indexIn(flags);
		if( reps == -1 )
			break;

		def = rx.cap(1);
		flags = flags.replace(QString("${%1}").arg(def), definitions.value(def));
	} while( reps != -1 );
	ret << flags;

	flags = attributes.value("Libs.private");
	do {
		reps = rx.indexIn(flags);
		if( reps == -1 )
			break;

		def = rx.cap(1);
		flags = flags.replace(QString("${%1}").arg(def), definitions.value(def));
	} while( reps != -1 );
	ret << flags;

	Pkg *ent;
	foreach( ent, depends )
	{
		ret << ent->getLibs();
	}
	return ret;
}

QStringList Pkg::getCFlags()
{
	QStringList ret;
	QRegExp rx("\\$\\{(\\w+)\\}");
	QString flags, def;
	int reps = -1;

	flags = attributes.value("Cflags");
	do {
		reps = rx.indexIn(flags);
		if( reps == -1 )
			break;

		def = rx.cap(1);
		flags = flags.replace(QString("${%1}").arg(def), definitions.value(def));
	} while( reps != -1 );
	ret << flags;

	flags = attributes.value("Cflags.private");
	do {
		reps = rx.indexIn(flags);
		if( reps == -1 )
			break;

		def = rx.cap(1);
		flags = flags.replace(QString("${%1}").arg(def), definitions.value(def));
	} while( reps != -1 );
	ret << flags;

	Pkg *ent;
	foreach( ent, depends )
	{
		ret << ent->getCFlags();
	}
	return ret;
}

bool PkgConfig::addPackage( QFileInfo fileInfo, QHash<QString, QString> attributes, QHash<QString, QString> definitions )
{
	Pkg *pkg = new Pkg(this);

	pkg->fileInfo = fileInfo;
	pkg->name = fileInfo.baseName();
	pkg->attributes = attributes;
	pkg->definitions = definitions;

	packages.append(pkg);

	return true;
}

bool PkgConfig::parseContent( QFileInfo fileInfo, QString content )
{
	QHash<QString, QString>	definitions;
	QHash<QString, QString> attributes;

	QRegExp whiteSpace("^(?:#.*|\\s*)$");

	QStringList lines = content.split('\n');
	QString line;
	int lineNum = 1;
	foreach( line, lines )
	{
		if( whiteSpace.exactMatch(line) )
		{
			lineNum++;
			continue;
		}

		int cSep = line.indexOf(": ");
		if( cSep == -1 )
			cSep = line.indexOf(":");

		if( cSep != -1 )
		{
			QString key = line.left( cSep );
			QString val = line.mid( cSep + 2 );

			attributes.insert(key, val);
			continue;
		}

		cSep = line.indexOf("=");
		if( cSep != -1 )
		{
			QString key = line.left( cSep );
			QString val = line.mid( cSep + 1 );

			definitions.insert(key, val);
			continue;
		}

		warning(QString("File '%1' contains unrecognized content around line %2: %3").arg(fileInfo.filePath()).arg(lineNum).arg(line));
		lineNum++;
	}

	if( definitions.size() == 0 && attributes.size() == 0 )
	{
		warning(QString("File '%1' didn't declare any attributes or definitions.").arg(fileInfo.filePath()));
		return false;
	}

	return addPackage( fileInfo, attributes, definitions );
}

bool PkgConfig::addPCFile(QFileInfo fileInfo)
{
	QFile file(fileInfo.filePath());
	file.open(QIODevice::ReadOnly);
	QByteArray content = file.readAll();
	if( content.isEmpty() )
	{
		warning(QString("File '%1' is either empty or contains no useful information.").arg(fileInfo.filePath()));
		return false;
	}

	return parseContent( fileInfo, content.data() );
}

bool PkgConfig::resolvePackages()
{
	Pkg *pkg;
	foreach( pkg, packages )
	{
		pkg->resolvePackageRequires();
		pkg->resolvePackageConflicts();

	}
	return true;
}

bool PkgConfig::loadPCFiles()
{
	bool ret = false;
	QString dirname;
	foreach( dirname, paths )
	{
		QDir dir(dirname);
		QFileInfoList list = dir.entryInfoList();
		for( int i=0; i < list.size(); i++ )
		{
			QFileInfo info = list.at(i);
			if( info.isFile() ) {
				if( info.fileName().right(3) != ".pc" )
				{
					warning(QString("File '%1' does not use the .pc file extension.").arg(info.filePath()));
				}
	
//				printf("loading '%s'...\n", info.filePath().toStdString().c_str() );
				if( addPCFile(info) )
				{
					ret = true;
				}
			}
		}
	}
	return ret;
}

bool PkgConfig::scanPath(QString path, int recurse)
{
	if( recurse > 32 )
	{
		error(QString("Recursed 32-levels into '%1', giving up.").arg(path));
		return false;
	}

	if( paths.contains(path) )
	{
		warning(QString("We already have '%1', skipping...").arg(path));
		return false;
	}

	bool ret = false;
	QDir dir(path);
	if( !dir.exists() )
	{
		warning(QString("Path '%1' is configured, but doesn't exist (or is unreadable).").arg(path));
		return false;
	}

	paths.append( path );

	QFileInfoList list = dir.entryInfoList();
	for( int i=0; i < list.size(); i++ )
	{
		QFileInfo info = list.at(i);
		if( info.isDir() )
		{
			if( !paths.contains(info.filePath()) )
				if( scanPath( info.filePath(), recurse+1 ) )
					ret = true;
		}
	}
	return ret;
}

bool PkgConfig::scanPaths()
{
	bool ret = false;
	QString path;
	foreach( path, paths )
	{
		if( scanPath(path, 0) )
			ret = true;
	}
	return ret;
}

bool PkgConfig::readPaths()
{
	int size = settings->beginReadArray("paths");
	paths.clear();
	for( int i=0; i < size; i++ )
	{
		settings->setArrayIndex(i);
		paths.append(settings->value("path").toString());
	}
	settings->endArray();
	return paths.size() > 0 ? true : false;
}

/*
prefix=/usr
exec_prefix=${prefix}
libdir=${prefix}/lib
includedir=${prefix}/include

Name: libavformat
Description: FFmpeg container format library
Version: 52.61.0
Requires:
Requires.private: libavcodec = 52.66.0
Conflicts:
Libs: -L${libdir} -lavformat
Libs.private: -lz -lbz2 -pthread -lm -lfaac -lfaad -lmp3lame -lm -lopencore-amrnb -lm -lopencore-amrwb -lm -ltheoraenc -ltheoradec -logg -lvorbisenc -lvorbis -logg -lx264 -lm -lxvidcore -lasound -ldl -lasound -lX11 -lXext -lXfixes -lasound
Cflags: -I${includedir}
~*/
