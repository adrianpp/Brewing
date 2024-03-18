#/bin/sh

pkg_install() {
	PKG=$1
	INSTALL_CMD=$2
	PKG_INSTALLED=$(dpkg-query -W --showformat='${Status}\n' $PKG | grep "install ok installed")
	if [ "" = "$PKG_INSTALLED" ]; then
		#need to install
		if [ "" = "$INSTALL_CMD" ]; then
			sudo apt-get --yes install $PKG
		else
			eval $INSTALL_CMD
		fi
	fi
}

pkg_install libboost-all-dev

