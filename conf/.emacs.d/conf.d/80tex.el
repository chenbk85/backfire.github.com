;; 最后更新: 2010年1月13日 21:27:24 by smyang

(add-to-list 'load-path "~/.emacs.d/site-lisp/auctex" t)
(add-to-list 'load-path "~/.emacs.d/site-lisp/auctex/preview" t)

(load "auctex.el" nil t t)
(load "preview-latex.el" nil t t)

(add-hook 'LaTeX-mode-hook 'turn-off-auto-fill)
;;tex环境下不要auto-fill

(custom-set-variables
  ;; custom-set-variables was added by Custom.
  ;; If you edit it by hand, you could mess it up, so be careful.
  ;; Your init file should contain only one such instance.
  ;; If there is more than one, they won't work right.
 '(TeX-PDF-mode t)
 '(TeX-save-query nil)
 '(TeX-show-compilation t)
 '(TeX-engine (quote xetex)))
