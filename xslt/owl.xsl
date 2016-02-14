<?xml version="1.0"?>
<xsl:stylesheet version="1.0" xmlns:xsl="http://www.w3.org/1999/XSL/Transform">
<xsl:output method="text" indent="no"/>
<xsl:variable name="lcase">abcdefghijklmnopqrstuvwxyz</xsl:variable>
<xsl:variable name="ucase">ABCDEFGHIJKLMNOPQRSTUVWXYZ</xsl:variable>

<xsl:template match="/">
  <xsl:value-of select="/ladspa/global/code"/>
  <xsl:apply-templates select="ladspa/plugin"/>
</xsl:template>

<xsl:template match="plugin">

<xsl:variable name="pluginLabel"><xsl:value-of select="@label"/></xsl:variable>
<xsl:variable name="PluginLabel"><xsl:call-template name="initialCaps"><xsl:with-param name="in" select="$pluginLabel" /></xsl:call-template></xsl:variable>
<xsl:variable name="PLUGINLABEL"><xsl:call-template name="allCaps"><xsl:with-param name="in" select="$pluginLabel" /></xsl:call-template></xsl:variable>
#include "StompBox.h"

typedef float LADSPA_Data;
inline int isnan(float x){
  return std::isnan(x);
}

/**
  <xsl:value-of select="$PluginLabel"/>
  <xsl:apply-templates select="/ladspa/global/meta"/>
  <xsl:apply-templates select="p"/>
*/
class <xsl:value-of select="$PluginLabel"/>Patch : public Patch {
private:
<xsl:apply-templates select="port" mode="declare"/>
<xsl:apply-templates select="instance-data" mode="declare"/>
public:
  <xsl:value-of select="$PluginLabel"/>Patch(){
    float s_rate = getSampleRate();
<xsl:apply-templates select="callback[@event='instantiate']"/>
  }

  void processAudio(AudioBuffer&amp; _buf){
    uint32_t sample_count = _buf.getSize();
    float s_rate = getSampleRate();
    input = _buf.getSamples(LEFT_CHANNEL);
    output = input;
    <xsl:value-of select="$PluginLabel"/>Patch* plugin_data = this;    
<xsl:apply-templates select="callback[@event='run']"/>
  }
};
</xsl:template>

<xsl:template match="callback">
<xsl:value-of select="."/>
</xsl:template>

<xsl:template match="port[@type='control']" mode="declare">
  <xsl:value-of select="concat('  float ', @label)"/>;
</xsl:template>

<xsl:template match="port[@type='audio']" mode="declare">
  <xsl:value-of select="concat('  float* ', @label)"/>;
</xsl:template>

<xsl:template match="instance-data" mode="declare">
  <xsl:value-of select="concat('  ', @type, ' ', @label)"/>;
</xsl:template>

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
