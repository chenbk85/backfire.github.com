;; 最后更新: 2013年1月4日 17:38:35 by smyang

(setq org-hide-leading-stars t)
(define-key global-map "\C-ca" 'org-agenda)
(setq org-log-done 'time)

(defcustom org-export-html-style
"<link rel=\"stylesheet\" type=\"text/css\" href=\"./base.css\">" ""
  :group 'org-export-html
  :type 'string)


;; export to Tex or PDF

(setq org-export-latex-coding-system 'utf-8)

(setq org-export-latex-date-format "%Y/%m/%d")

(setq org-export-latex-default-class "article")

(setq org-export-latex-classes '(("article" "% !TEX TS-program = xelatex\n% !TEX encoding = UTF-8\n\\documentclass{article}\n\\usepackage{xunicode}\n\\usepackage[slantfont,boldfont]{xeCJK}\n\\setmainfont{Monaco}\n\\setCJKmainfont[BoldFont={WenQuanYi Zen Hei}]{WenQuanYi Micro Hei}\n\\setCJKmonofont{WenQuanYi Micro Hei Mono}"
;; \n\n\\usepackage[a4paper]{geometry}\n\\usepackage{graphicx} % support the \includegraphics command and options\n\\usepackage[colorlinks=true,linkcolor=blue]{hyperref}"

("\\section{%s}" . "\\section*{%s}")

("\\subsection{%s}" . "\\subsection*{%s}")

("\\subsubsection{%s}" . "\\subsubsection*{%s}")

("\\paragraph{%s}" . "\\paragraph*{%s}")

("\\subparagraph{%s}" . "\\subparagraph*{%s}"))

;; ("report" "% !TEX TS-program = xelatex\n% !TEX encoding = UTF-8\n\\documentc
;; lass[11pt,titlepage]{report} % use larger type; default would be 10pt\n\\use
;; package{xeCJK}\n\\setmainfont{AdobeHebrew-Italic}\n\\setCJKmainfont{Adobe 宋
;; 体 Std}\n\\setCJKfamilyfont{Adobe 宋体 Std}{AR PL SungtiL GB}\n\n\\usepackag
;; e[a4paper]{geometry}\n\\usepackage{graphicx} % support the \includegraphics
;; command and options\n\\usepackage[colorlinks=true,linkcolor=blue]{hyperref}"


;; ("\\part{%s}" . "\\part*{%s}")

;; ("\\chapter{%s}" . "\\chapter*{%s}")

;; ("\\section{%s}" . "\\section*{%s}")

;; ("\\subsection{%s}" . "\\subsection*{%s}")

;; ("\\subsubsection{%s}" . "\\subsubsection*{%s}"))

;; ("book" "% !TEX TS-program = xelatex\n% !TEX encoding = UTF-8\n\\documentcla
;; ss[11pt,titlepage]{book} % use larger type; default would be 10pt\n\\usepack
;; age{xeCJK}\n\\setmainfont{AdobeHebrew-Italic}\n\\setCJKmainfont{Adobe 宋体 S
;; td}\n\\setCJKfamilyfont{Adobe 宋体 Std}{AR PL SungtiL GB}\n\n\\usepackage[a4
;; paper]{geometry}\n\\usepackage{graphicx} % support the \includegraphics comm
;; and and options\n\\usepackage[colorlinks=true,linkcolor=blue]{hyperref}"

;; ("\\part{%s}" . "\\part*{%s}")

;; ("\\chapter{%s}" . "\\chapter*{%s}")

;; ("\\section{%s}" . "\\section*{%s}")

;; ("\\subsection{%s}" . "\\subsection*{%s}")

;; ("\\subsubsection{%s}" . "\\subsubsection*{%s}"))
))
