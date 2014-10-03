;; $Id: pari-conf.el.in 5072 2003-11-20 18:49:46Z kb $
;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
(provide 'pari-conf)
;; Set the following constants for your site:
(defconst gp-version "2.3.4" "pari's version number")

(defconst gp-file-name "/usr/local/bin/gp"
 "The file name of the gp executable file")

(defconst gp-gphelp-dir "/usr/local/bin/"
  "The directory where gphelp is to be found")

(defconst gp-pariemacs "@emacsdir/pariemacs.txt"
  "The pariemacs file")
