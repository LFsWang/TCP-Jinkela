# Jinkela FTP Proxy
�o�O�Q��C++11���᪺�t�γ̰���ǫ�clock���X�Ӫ��t�v����FTP Proxy�A��make�sĶ��N�i�H�ϥΤF�A�ѩ�make�O��c++14�Ӷi��sĶ���A�ҥH�p�G����sĶ���ܽбNg++��s��̷s�����C

�쥻�bdemo�ɧڭ�**�u��**�L�]������A�]�N�O25Kbps�W�Ǫ����ӡA��L���W�ǭ��t���O�����A���O�bubuntu�W���O�諸�A��ӵo�{�OOS�����D�A�ڭ̥ΤF�@�Ө�Ʒ|�b���P���q���U�����P�����G�A��O�ڭ̱N��i��@�ǭק�A�̫�ש󧹦��{�b�o�Ӫ����C

## �Ѽƻ���
��J�榡��: ```<Proxy IP> <Proxy Port> <�U���t�v> <�W�ǳt�v>```

�W�ǻP�U���t�v����쳣�OKbps�A�B������int�d����

�d��

  * ./a.out 127.0.0.1 8888 25 25

## �S����
�ڭ̨ϥ�setsockopt�o�Ө�ƨӱ����J�w�X��(SO_RCVBUF)���j�p�A���O�bubuntu�USO_RCVBUF�̤p�|�O2304 byte�A����A�p�F�A�@�}�l�ڭ̳]�w�L�O8 byte�C���O�U��demo�ɥΪ��Omac�Amac�������|�h����SO_RCVBUF���j�p�A�ҥH�N�u����SO_RCVBUF�ܦ�8 byte�A�ҥH�ڭ̤W�Ǫ��t�v�ܦ��̤j�u��O90�A���Odemo�ɪ��W�ǳt�v��100��500�A�ҥH�N��F�C

���L���ξ�ߡA�o�Ӫ����w�g�վ�]�wSO_RCVBUF��1024�A�]���t�η|�۰ʧ⥦���G�ܦ�2048�A�P2304�w�g�Q������ҥH���|�����D�C