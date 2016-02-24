<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" indent="no"/>
<xsl:variable name="lcase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
<xsl:variable name="ucase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

<xsl:template match="/">
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}
  <xsl:value-of select="/ladspa/global/code"/>
  <xsl:apply-templates select="ladspa/plugin"/>
</xsl:template>

<xsl:template match="plugin">
<xsl:variable name="pluginLabel"><xsl:value-of select="@label"/></xsl:variable>
<xsl:variable name="PluginLabel"><xsl:call-template name="initialCaps"><xsl:with-param name="in" select="$pluginLabel" /></xsl:call-template></xsl:variable>
<xsl:variable name="PLUGINLABEL"><xsl:call-template name="allCaps"><xsl:with-param name="in" select="$pluginLabel" /></xsl:call-template></xsl:variable>

/**
  <xsl:value-of select="$PluginLabel"/>
  <xsl:apply-templates select="/ladspa/global/meta"/>
  <xsl:apply-templates select="p"/>
*/
class <xsl:value-of select="$PluginLabel"/>Patch : public Patch {
private:
<xsl:apply-templates mode="declare"/>
public:
  <xsl:value-of select="$PluginLabel"/>Patch(){
<xsl:apply-templates mode="register"/>
    float s_rate = getSampleRate();
<xsl:apply-templates select="callback[@event='instantiate']"/>
  }

  void processAudio(AudioBuffer&amp; _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
<xsl:apply-templates select="port" mode="assign"/>
<xsl:value-of select="$PluginLabel"/>Patch* plugin_data = this;    
<xsl:apply-templates select="callback[@event='run']"/>
  }
};
</xsl:template>

<xsl:template match="callback">
<xsl:value-of select="."/>
</xsl:template>

<xsl:template match="port[@type='control']" mode="declare">
  float <xsl:if test="@dir='output'">*</xsl:if>
  <xsl:value-of select="@label"/>;
</xsl:template>

<xsl:template match="port[@type='audio']" mode="declare">
  <xsl:value-of select="concat('  float* ', @label)"/>;
</xsl:template>

<xsl:template match="instance-data" mode="declare">
  <xsl:value-of select="concat('  ', @type, ' ', @label)"/>;
</xsl:template>

<xsl:template match="port[@type='control'][@dir='input'][position() &lt; 6]" mode="register">
  <xsl:text>  registerParameter(PARAMETER_</xsl:text>
  <xsl:value-of select="translate(count(preceding::port[@type='control'])+1, '12345', 'ABCDE')"/>
  <xsl:text>, "</xsl:text>
  <xsl:value-of select="name"/>
  <xsl:text>");
</xsl:text>
</xsl:template>

<xsl:template match="port[@type='control'][@dir='input'][position() &lt; 6]" mode="assign">
  <xsl:value-of select="concat('  ', @label, ' = getParameterValue(PARAMETER_')"/>
  <xsl:value-of select="translate(position(), '12345', 'ABCDE')"/>
  <xsl:text>)</xsl:text>
  <xsl:apply-templates select="range" mode="assign"/>;
</xsl:template>

<xsl:template match="range" mode="assign">
  <xsl:text>*</xsl:text>
  <xsl:value-of select="@max - @min"/> - <xsl:value-of select="@min"/>
</xsl:template>

<xsl:template match="port[@type='audio']" mode="assign">
  <xsl:value-of select="concat('  ', @label, ' = _buf.getSamples(')"/>
  <xsl:value-of select="count(preceding::port[@type='audio'][@dir=current()/@dir])"/>);
</xsl:template>

<xsl:template match="text()|*" mode="declare"/>
<xsl:template match="text()|*" mode="register"/>
<xsl:template match="text()|*" mode="assign"/>

<xsl:template match="p">
<xsl:value-of select="."/>
</xsl:template>

<xsl:template match="meta[@name='maker']">
  By <xsl:value-of select="@value"/>.
</xsl:template>

<xsl:template match="meta[@name='copyright']">
  Published under the <xsl:value-of select="@value"/> license.
</xsl:template>

<xsl:template name="initialCaps">
  <xsl:param name="in" />
  <xsl:variable name="f" select="substring($in, 1, 1)" />
  <xsl:variable name="r" select="substring($in, 2)" />
  <xsl:value-of select="concat(translate($f, $lcase, $ucase),$r)"/>
</xsl:template>

<xsl:template name="allCaps">
  <xsl:param name="in" />
  <xsl:value-of select="translate($in, $lcase, $ucase)"/>
</xsl:template>

</xsl:stylesheet>
