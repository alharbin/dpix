/*****************************************************************************\

NPRStyle.cc
Copyright (c) 2009 Forrester Cole

libnpr is distributed under the terms of the GNU General Public License.
See the COPYING file for details.

\*****************************************************************************/

#include <NPRStyle.h>
#include <NPRSettings.h>
#include <stdio.h>
#include <assert.h>

#include <QTextStream>
#include <QStringList>
#include <QDomDocument>
#include <QDomText>
#include <QFile>
#include <QMessageBox>

const int CURRENT_VERSION = 4;

const char* TRANSFER_NAMES[NPRStyle::NUM_TRANSFER_FUNCTIONS] = 
{
    "focus_transfer",
    "line_color",
    "line_opacity",
    "line_texture",
    "line_elision",
    "fill_fade",
    "fill_desat",
    "paper_params",
};

//
// Helper functions
//

void loadVec( vec& v, const QDomElement& element )
{
	QString data = element.text();
	QTextStream stream(&data);
	stream >> v[0] >> v[1] >> v[2];
}

void saveVec( vec& v, const QString& name, QDomDocument& doc, QDomElement& parent )
{
	QDomElement element = doc.createElement("vec3");
	QString s = QString("%1 %2 %3").arg(v[0]).arg(v[1]).arg(v[2]);
	QDomText text = doc.createTextNode(s);
	element.appendChild(text);
	element.setAttribute("name", name);
	parent.appendChild(element);
}

void saveFloat( float f, const QString& name, QDomDocument& doc, QDomElement& parent )
{
	QDomElement fvalue = doc.createElement("float");
	fvalue.setAttribute("value", f);
	fvalue.setAttribute("name", name);
	parent.appendChild(fvalue);
}

void saveInt( int i, const QString& name, QDomDocument& doc, QDomElement& parent )
{
	QDomElement ivalue = doc.createElement("int");
	ivalue.setAttribute("value", i);
	ivalue.setAttribute("name", name);
	parent.appendChild(ivalue);
}

void appendTextNode( QDomDocument& doc, QDomElement& parent, const QString& name, const QString& text )
{
    QDomElement element = doc.createElement(name);
	QDomText node = doc.createTextNode(text);
	element.appendChild(node);
	parent.appendChild(element);
}

//
// NPRTransfer
//

void NPRTransfer::load( const QDomElement& element )
{
	QString t = element.text();
	QTextStream stream(&t);
	stream >> v1 >> v2 >> vnear >> vfar;
}

void NPRTransfer::save( QDomDocument& doc, QDomElement& element )
{
	QString t;
	QTextStream stream(&t);
	stream << v1 << " " << v2 << " " << vnear << " " << vfar;

	QDomText textnode = doc.createTextNode(t);
	element.appendChild(textnode);
}

void NPRTransfer::print() const
{
  printf("%f %f %f %f\n", v1, v2, vnear, vfar);
}

float NPRTransfer::compute( float f ) const
{
  if (vnear > vfar)
    return v1;
  else if (f < vnear)
    return v1;
  else if (f > vfar)
    return v2;
  else
    {
      float alpha = (f - vnear) / (vfar - vnear);
      return v1 + (v2 - v1)*alpha;
    }           
}

void NPRTransfer::toArray( float ar[4]) const
{
    ar[0] = v1;
    ar[1] = v2;
    ar[2] = vnear;
    ar[3] = vfar;
}

void NPRTransfer::toArray( float ar[4], float v1_multiple, float v2_multiple) const
{
    toArray(ar);
    ar[0] = (1.0f - v1) * v1_multiple + v1 * v2_multiple;
    ar[1] = (1.0f - v2) * v1_multiple + v2 * v2_multiple;
}

void NPRTransfer::set( float v1, float v2, float vnear, float vfar )
{
        this->v1 = v1;
        this->v2 = v2;
        this->vnear = vnear;
        this->vfar = vfar;
}

//
// NPRPenStyle
//

NPRPenStyle::NPRPenStyle()
{
    _texture = NULL;
    _color = vec(0.1f, 0.1f, 0.1f);
    _opacity = 1.0f;
    _strip_width = 1.0f;
    _length_scale = 1.0f;
    _elision_width = 1.0f;
}

void NPRPenStyle::clear()
{
    if (_texture)
    {
        delete _texture;
        _texture = NULL;
    }
	_texture_file = QString();
}

void NPRPenStyle::copyFrom( const NPRPenStyle& style )
{
    *this = style;
    // make a new copy of the texture
    _texture = NULL;
    setTexture(_texture_file);
}

void NPRPenStyle::load( const QDir& style_dir, const QDomElement& element )
{
    _name = element.attribute("name");

    QDomElement c = element.firstChildElement("color");
	assert(!c.isNull());
    loadVec(_color, c);

    QString rel_name = element.firstChildElement("texture").text();
    _texture_file = style_dir.absoluteFilePath(rel_name);
    _opacity = element.firstChildElement("opacity").text().toFloat();
    _strip_width = element.firstChildElement("strip_width").text().toFloat();
    _elision_width = element.firstChildElement("elision_width").text().toFloat();
    _length_scale = element.firstChildElement("length_scale").text().toFloat();

    bool ret = setTexture(_texture_file);
    if (!ret)
    {
        _texture = NULL;
    }
}

void NPRPenStyle::save( const QDir& style_dir, QDomDocument& doc, QDomElement& element )
{
    element.setAttribute("name", _name);

    appendTextNode(doc, element, QString("color"), QString("%1 %2 %3").arg(_color[0]).arg(_color[1]).arg(_color[2]));
    QString rel_name = style_dir.relativeFilePath(_texture_file);
    appendTextNode(doc, element, QString("texture"), rel_name);
    appendTextNode(doc, element, QString("opacity"), QString("%1").arg(_opacity));
    appendTextNode(doc, element, QString("strip_width"), QString("%1").arg(_strip_width));
    appendTextNode(doc, element, QString("elision_width"), QString("%1").arg(_elision_width));
    appendTextNode(doc, element, QString("length_scale"), QString("%1").arg(_length_scale));
}

bool NPRPenStyle::setTexture( const QString& filename )
{
    if (filename.isEmpty())
    {
        return false;
    }

    if (!QFileInfo(filename).exists())
    {
        QMessageBox::critical(NULL, "Open Failed", 
            QString("Could not find pen texture \"%1\"").arg(filename));
        return false;
    }

    GQTexture* new_texture = new GQTexture2D();

    if (!new_texture->load(filename))
    {
        QMessageBox::critical(NULL, "Open Failed", 
            QString("Could not load pen texture \"%1\" (corrupt texture?)").arg(filename));
        delete new_texture;
        return false;
    }

    if (_texture)
    {
        delete _texture;
    }
    _texture = new_texture;
    _texture_file = filename;

    return true;
}


// NPRStyle

NPRStyle::NPRStyle()
{
    _paper_texture = NULL;
    _background_texture = NULL;
    loadDefaults();
}

void NPRStyle::clear()
{
    for (uint32 i = 0; i < _pen_styles.size(); i++)
        delete _pen_styles[i];
    _pen_styles.clear();

    if (_paper_texture)
    {
        delete _paper_texture;
        _paper_texture = NULL;
    }
    _paper_file = QString();

    if (_background_texture)
    {
        delete _background_texture;
        _background_texture = NULL;
    }
    _background_file = QString();   
    _path_style_dirty = true;
}

void NPRStyle::loadDefaults()
{
    clear();

    // Some reasonable default values.
    _background_color = vec(1.0f, 1.0f, 1.0f);
    
    _transfers[LINE_COLOR].set(0.0f, 1.0f, 0.0f, 1.0f);
    _transfers[LINE_OPACITY].set(0.0f, 1.0f, 0.0f, 1.0f);
    _transfers[LINE_TEXTURE].set(0.0f, 1.0f, 0.0f, 1.0f);
    _transfers[LINE_ELISION].set(0.0f, 1.0f, 0.0f, 1.0f);
    
    _transfers[FILL_FADE].set(0.4f, 0.8f, 0.0f, 1.0f);
    _transfers[FILL_DESAT].set(0.0f, 0.8f, 0.0f, 1.0f);
    
    _transfers[FOCUS_TRANSFER].set(0.0f, 1.0f, 0.2f, 0.8f);

    _transfers[PAPER_PARAMS].set(0.15f, 0.0f, 0.0f, 1.0f);

    _pen_styles.push_back( new NPRPenStyle() );
    _pen_styles[0]->setName("Base Style");

    _draw_invisible_lines = false;
    _enable_line_elision = false;
}


bool NPRStyle::load( const QString& filename )
{
	QFile file(filename);
    QDir dir(filename);
	if (!file.open(QIODevice::ReadOnly))
	{
		qWarning("NPRStyle::load - Could not open %s", qPrintable(filename));
		return false;
	}

	QDomDocument doc("npr_style");

	QString parse_errors;
	if (!doc.setContent(&file, &parse_errors))
	{
		file.close();
        // Could be a version 1 file, but we don't read those
        // anymore. 
	    qWarning("NPRStyle::load - Failed to read %s. Parse errors: %s\n", qPrintable(filename), qPrintable(parse_errors));
        return false;
	}

	file.close();

    clear();

	QDomElement root = doc.documentElement();

    return load(dir, root);
}

bool NPRStyle::load( const QDir& style_dir, const QDomElement& root )
{
    clear();

	QDomElement header = root.firstChildElement("header");
	assert(!header.isNull());
	QDomElement version = header.firstChildElement("version");
	assert(!version.isNull());

	int version_num = version.text().toInt();
	if (version_num < CURRENT_VERSION)
	{
		qWarning("NPRStyle::load - Obsolete file version %d (current is %d)", version_num, CURRENT_VERSION );
		return false;
	}

    // pen styles
	QDomElement pen_styles = root.firstChildElement("pen_styles");
	QDomElement style_element = pen_styles.firstChildElement("pen_style");
	while (!style_element.isNull())
	{
        NPRPenStyle* penstyle = new NPRPenStyle();
        penstyle->load( style_dir, style_element );
		style_element = style_element.nextSiblingElement("pen_style");
        _pen_styles.push_back(penstyle);
	}

    if (_pen_styles.empty())
    {
        _pen_styles.push_back( new NPRPenStyle() );
        _pen_styles[0]->setName("Base Style");
    }

	// read texture files
	QDomElement textures = root.firstChildElement("textures");
	QDomElement texfile = textures.firstChildElement("texture");
	while (!texfile.isNull())
	{
		QString type = texfile.attribute("name");
		if (type == "pen")
            _pen_styles[0]->setTexture(texfile.attribute("file"));
		else if (type == "paper")
            loadPaperTexture(style_dir.absoluteFilePath(texfile.attribute("file")));
		else if (type == "background")
            loadBackgroundTexture(style_dir.absoluteFilePath(texfile.attribute("file")));
		else
			assert(0);

		texfile = texfile.nextSiblingElement("texture");
	}

	// transfer functions
	QDomElement transfer_functions = root.firstChildElement("transfer_functions");
	QDomElement trans = transfer_functions.firstChildElement("vec4");
	while (!trans.isNull())
	{
		NPRTransfer* tfunc = transferByName( trans.attribute("name") );
        if (tfunc)
        {
    		tfunc->load(trans);
        }
		trans = trans.nextSiblingElement("vec4");
	}

	// colors
	QDomElement colors = root.firstChildElement("colors");
	QDomElement color = colors.firstChildElement("vec3");
	while (!color.isNull())
	{
		QString name = color.attribute("name");
		if (name == "pen") 
        {
            vec pc;
            loadVec( pc, color );
            _pen_styles[0]->setColor( pc );
        }
		else if (name == "background") loadVec( _background_color, color );

		color = color.nextSiblingElement("vec3");
	}

    // ivalues
    QDomElement params = root.firstChildElement("parameters");
    QDomElement ivalue = params.firstChildElement("int");
    while (!ivalue.isNull())
    {
        QString name = ivalue.attribute("name");
        int value = ivalue.attribute("value").toInt();

        if (name == "draw_invisible_lines") _draw_invisible_lines = (bool)value;
        if (name == "enable_line_elision") _enable_line_elision = (bool)value;

        ivalue = ivalue.nextSiblingElement("int");
    }

	return true;
}


bool NPRStyle::save( const QString& filename )
{
    QDomDocument doc("npr_style");
	QDomElement root = doc.createElement("npr_style");
	doc.appendChild(root);

	QFile file(filename);
    QDir dir(filename);
	if (!file.open(QIODevice::WriteOnly))
	{
		qWarning("NPRStyle::save - Could not save %s", qPrintable(filename));
		return false;
	}

    bool ret = save(dir, doc, root);
    if (!ret)
    {
        file.close();
        return false;
    }

	file.write(doc.toByteArray());

	file.close();

	return true;
}

bool NPRStyle::save( const QDir& style_dir, QDomDocument& doc, QDomElement& root )
{
	QDomElement header = doc.createElement("header");
	root.appendChild(header);

	QDomElement version = doc.createElement("version");
	QDomText versiontext = doc.createTextNode( QString("%1").arg(CURRENT_VERSION) );
	header.appendChild(version);
	version.appendChild(versiontext);

	// textures
	QDomElement textures = doc.createElement("textures");
	root.appendChild(textures);
    QDomElement texfile;
	if (!_paper_file.isEmpty())
	{
		texfile = doc.createElement("texture");
		texfile.setAttribute("name", "paper");
        QString rel_name = style_dir.relativeFilePath(_paper_file);
		texfile.setAttribute("file", rel_name);
		textures.appendChild(texfile);
	}
	if (!_background_file.isEmpty())
	{
		texfile = doc.createElement("texture");
		texfile.setAttribute("name", "background");
        QString rel_name = style_dir.relativeFilePath(_paper_file);
		texfile.setAttribute("file", rel_name);
		textures.appendChild(texfile);
	}

	// transfer functions
	QDomElement transfer_functions = doc.createElement("transfer_functions");
	root.appendChild(transfer_functions);
	
	for (int i = 0; i < NUM_TRANSFER_FUNCTIONS; i++)
	{
		QDomElement trans = doc.createElement("vec4");
		_transfers[i].save( doc, trans );
		trans.setAttribute("name", TRANSFER_NAMES[i]);
		transfer_functions.appendChild(trans);
	}

    // pen styles
	QDomElement pen_styles = doc.createElement("pen_styles");
    root.appendChild(pen_styles);
    for (uint32 i = 0; i < _pen_styles.size(); i++)
    {
        QDomElement penstyle = doc.createElement("pen_style");
        _pen_styles[i]->save( style_dir, doc, penstyle );
        pen_styles.appendChild(penstyle);
    }

	// colors
	QDomElement colors = doc.createElement("colors");
	root.appendChild(colors);
	saveVec( _background_color, "background", doc, colors );

	// parameters
	QDomElement params = doc.createElement("parameters");
	root.appendChild(params);
    saveInt((int)_draw_invisible_lines, "draw_invisible_lines", doc, params);
    saveInt((int)_enable_line_elision, "enable_line_elision", doc, params);

    return true;
}

bool NPRStyle::loadPaperTexture(const QString& filename)
{
    return loadTexture(_paper_file, _paper_texture, "paper", filename);
}

bool NPRStyle::loadBackgroundTexture(const QString& filename)
{
    return loadTexture(_background_file, _background_texture, "background", filename);
}

bool NPRStyle::loadTexture(QString& output_name, GQTexture*& output_ptr, 
                           const QString& field_name, const QString& filename)
{
    if (filename.isEmpty())
    {
        qWarning("NPRStyle::loadTexture - Warning: blank %s texture filename.\n", qPrintable(field_name));
        output_ptr = NULL;
        output_name = QString();
        return true;
    }

    if (!QFileInfo(filename).exists())
    {
        QMessageBox::critical(NULL, "Open Failed", 
            QString("Could not find %1 texture \"%2\"").arg(field_name).arg(filename));
        return false;
    }

    GQTexture2D* new_texture = new GQTexture2D();
    if (!new_texture->load(filename))
    {
        QMessageBox::critical(NULL, "Open Failed", 
            QString("Could not load %1 texture \"%2\" (corrupt texture?)").arg(field_name).arg(filename));
        delete new_texture;
        return false;
    }

    if (output_ptr)
    {
        delete output_ptr;
    }
    output_ptr = new_texture;
	output_name = filename;
    
    return true;
}



NPRTransfer* NPRStyle::transferByName( const QString& name )
{
    NPRTransfer* func = 0;
    for (int i = 0; i < NUM_TRANSFER_FUNCTIONS; i++)
    {
        if (TRANSFER_NAMES[i] == name)
        {
            func = &_transfers[i];
            break;
        }
    }

	return func;
}


NPRTransfer& NPRStyle::transferRef( NPRStyle::TransferFunc func )
{
    if (func == LINE_ELISION)
        _path_style_dirty = true;
    
    return _transfers[func];   
}

const NPRPenStyle* NPRStyle::penStyle(const QString& name) const
{
    for (uint32 j = 0; j < _pen_styles.size(); j++)
    {
        if (_pen_styles[j]->name() == name)
        {
            return _pen_styles[j];
        }
    }
    return _pen_styles[0];
}
NPRPenStyle* NPRStyle::penStyle(const QString& name)
{
    for (uint32 j = 0; j < _pen_styles.size(); j++)
    {
        if (_pen_styles[j]->name() == name)
        {
            return _pen_styles[j];
        }
    }
    return _pen_styles[0];
}

bool NPRStyle::hasPenStyle(const QString& name) const
{
    for (uint32 j = 0; j < _pen_styles.size(); j++)
    {
        if (_pen_styles[j]->name() == name)
        {
            return true;
        }
    }
    return false;
}

void NPRStyle::addPenStyle(NPRPenStyle* style)
{
    _pen_styles.push_back(style);
}

bool NPRStyle::deletePenStyle( int which )
{
    assert(which >= 0 && (uint32)which < _pen_styles.size());
    delete _pen_styles[which];
    _pen_styles.erase(_pen_styles.begin() + which);
    return true;
}

bool NPRStyle::deletePenStyle( const QString& name )
{
    for (uint32 j = 0; j < _pen_styles.size(); j++)
    {
        if (_pen_styles[j]->name() == name)
        {
            deletePenStyle(j);
            return true;
        }
    }
    return false;
}

