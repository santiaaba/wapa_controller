#!/bin/bash

cd /tmp

ftp_ip=10.120.78.193
filer_ip=10.120.78.181

#Inhabilitando selinux. En un futuro sera configurando selinux
ssh "root@$ftp_ip" "setenforse 0"
ssh "root@$ftp_ip" "sed -i -e 's/SELINUX=enforcing/SELINUX=disabled/' /etc/selinux/config"

# Agregar wget
ssh "root@$ftp_ip" "yum -y install wget"
if [ $? -new 0 ]; then
        echo "Error al querer instalar el wget"
        exit 1
fi

# agregar nfs-utils
ssh "root@$ftp_ip" "yum -y install nfs-utils"
if [ $? -ne 0 ]; then
        echo "Error al instalar nfs-utils"
        exit 1
fi

# Creando directorio de montaje
ssh "root@$ftp_ip" "mkdir /websites"
if [ $? -ne 0 ]; then
        echo "Error al crear directorio de montaje websites"
        exit 1
fi

# Generando entrada en fstab
ssh "root@$ftp_ip" "echo \"$filer_ip:/websites         /websites       nfs     defaults        0 0\" >> /etc/fstab"
if [ $? -ne 0 ]; then
        echo "Error al crear directorio de montaje websites"
        exit 1
fi

#Generando entrada en filer para permitir montaje del ftp
ssh "root@$filer_ip" "echo \"/websites       10.120.78.193(rw,sync,no_root_squash)\" >> /etc/exports; exportfs -a"

# Montando
ssh "root@$ftp_ip" "mount /websites"

# Descargando fuente proftpd
ssh "root@$ftp_ip" "wget ftp://ftp.proftpd.org/distrib/source/proftpd-1.3.6.tar.gz"
if [ $? -ne 0 ]; then
        echo "Error al descargar proftpd de su fuente"
        exit 1
fi

# Descomprimiendo fuentes del proftpd
ssh "root@$ftp_ip" "tar -xzf proftpd-1.3.6.tar.gz"
if [ $? -ne 0 ]; then
        echo "Error al descomprimir fuentes del proftpd"
        exit 1
fi

# Instalando proftpd
ssh "root@$ftp_ip" "cd proftpd-1.3.6;./configure --witzh-modules=mod_quotatab:mod_sql:mod_quotatab_sql:mod_sql_mysql;make;make install"
if [ $? -ne 0 ]; then
        echo "Error al descomprimir fuentes del proftpd"
        exit 1
fi

#Creando usuario y grupo
ssh "root@$ftp_ip" "groupadd -g 2001 ftpgroup"
ssh "root@$ftp_ip" "useradd -u 2001 -s /bin/false -d /bin/null -c \"proftpd wapa user\" -g ftpgroup ftpuser"

#Creamos los permisos en la base de datos
mysql -u root --password=sAn474226 --execute="GRANT SELECT, INSERT, UPDATE, DELETE ON ftp.* TO 'proftpd'@'$ftp_ip' IDENTIFIED BY 'sAn474226'"
if [ $? -ne 0 ]; then
        echo "Error al generar los accesos a la base de datos"
        exit 1
fi
mysql -u root --password=sAn474226 --execute="flush privileges"

# Copiar configuracion standard
scp ./proftpd.conf root@$ftp_ip:/usr/local/etc/proftpd.conf

# Modificar configuracion standard
ssh "root@$ftp_ip" "sed -i \"s/#DBNAME/$db_name/\" /usr/local/etc/proftpd.conf"
ssh "root@$ftp_ip" "sed -i \"s/#DBSERVER/$db_server/\" /usr/local/etc/proftpd.conf"
ssh "root@$ftp_ip" "sed -i \"s/#DBUSERFTP/$db_user/\" /usr/local/etc/proftpd.conf"
ssh "root@$ftp_ip" "sed -i \"s/#DBPASSFTP/$db_pass/\" /usr/local/etc/proftpd.conf"

#Instalando el ftp_tool
#Configurando el ftp_tool
