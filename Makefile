#
# This software is licensed under the Public Domain.
#

include $(TOPDIR)/rules.mk

PKG_NAME:=simple-rt-cli
PKG_VERSION:=0.2
PKG_RELEASE:=2

# see https://spdx.org/licenses/
PKG_LICENSE:=CC0-1.0

PKG_BUILD_DIR := $(BUILD_DIR)/$(PKG_NAME)


PLATFORM = linux
TARGET_CFLAGS += -I$(STAGING_DIR)/usr/include/libusb-1.0
TARGET_CFLAGS += -I$(PKG_BUILD_DIR)/include/
TARGET_CFLAGS += -DPLATFORM="\"$(PLATFORM)\""

include $(INCLUDE_DIR)/package.mk

define Package/simple-rt-cli
	SECTION:=net
	# Should this package be selected by default?
	#DEFAULT:=y
	CATEGORY:=Network
	TITLE:=Some other dummy application.
	# Feature FOO also needs libsodium:
	DEPENDS:=+librt +libusb-1.0 
	MAINTAINER:= Foo Bar <foo@example.com>
	URL:=https://www.example.com
	SUBMENU:=VPN
endef

define Package/simple-rt-cli/description
	Some example Programm called simple-rt-cli
endef

define Package/simple-rt-cli/config
	source "$(SOURCE)/Config.in"
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) -r ./src/* $(PKG_BUILD_DIR)/
	$(CP) -r ./include/ $(PKG_BUILD_DIR)/
endef

define Build/Configure
# Nothing to do here for us.
# By default simple-rt-cli/src/Makefile will be used.
endef

define Build/Compile
	CFLAGS="$(TARGET_CFLAGS) -D_DEFAULT_SOURCE -lm -lpthread -lresolv -lusb-1.0" \
	CPPFLAGS="$(TARGET_CPPFLAGS)" \
	LDLIBS="$(LDLIBS)" \
	$(MAKE) -C $(PKG_BUILD_DIR) \
		$(TARGET_CONFIGURE_OPTS)
endef

define Package/simple-rt-cli/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/simple-rt-cli $(1)/usr/sbin/
endef

$(eval $(call BuildPackage,simple-rt-cli,+libusb-1.0))
