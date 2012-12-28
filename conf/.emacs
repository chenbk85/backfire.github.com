;; 最后更新: 2010年12月22日 18:18:14 by smyang

(add-to-list 'load-path "~/.emacs.d/site-lisp" t)

(mapcar 'load 
	(directory-files "~/.emacs.d/conf.d" 
			 t "^[0-9][0-9].*[^-]\\.elc" nil))


(custom-set-variables
 ;; custom-set-variables was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 '(TeX-PDF-mode t)
 '(TeX-engine (quote xetex))
 '(TeX-save-query nil)
 '(TeX-show-compilation t)
 '(org-agenda-files (quote ("~/workspace/org/smyang.org")))
 '(org-latex-to-pdf-process (quote ("xelatex -interaction nonstopmode -output-directory %o %f" "xelatex -interaction nonstopmode -output-directory %o %f" "xelatex -interaction nonstopmode -output-directory %o %f"))))

(custom-set-faces
 ;; custom-set-faces was added by Custom.
 ;; If you edit it by hand, you could mess it up, so be careful.
 ;; Your init file should contain only one such instance.
 ;; If there is more than one, they won't work right.
 )
