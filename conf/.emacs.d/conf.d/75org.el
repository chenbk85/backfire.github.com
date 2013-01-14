;; 最后更新: 2013年1月9日 17:17:23 by smyang

(setq org-hide-leading-stars t)
(define-key global-map "\C-ca" 'org-agenda)
(setq org-log-done 'time)

(defcustom org-export-html-style
"<style type=\"text/css\">
body 
{
  width: 1080px;
  margin: auto;
  background-color: #fff;
  color: #333;
  font-family: Verdana, arial, helvetica, sans-serif; 
  font-size: 16px;
  margin-top: 10px;
  padding: 5px 20px;
  }

/* headings */
h1
{ 
  line-height: 0px;
 }
/*h2 
{ 
  font-size : 110%;
  font-weight : normal;
  border-bottom : 1px solid #444;
  display : inline;
}*/

/* faces */
b 
{ 
  color:#24306f;
  }

/* h rule */
hr 
{
  background-color : #333;
  height : 1px;
  border : 0;
  }

/* list env */
li
{ 
  margin-bottom: .5em;
 }

ul.plain
{ 
  /*display: table;*/
  list-style: none;
  width: 660px
 }

ul.plain li
{ 
/*  display: table-cell;*/
  float: left;
  width: 220px;
  }

/* description env, for paper list*/
dl
{
  width: 720px;
  }
dt
{ 
  width: 40px;
  margin: 0;
  /*display: table-cell;*/
  float:left;
  }

dd
{ 
  width: 675px;
  float:left;
  margin: 0;
  margin-bottom: 1em;
/*  display: table-cell;*/
  }


/* anchor */
a 
{
  color : #24306F /*#214dff*/ ;		
  text-decoration : underline;
  }

a:hover 
{
  color : #ce4542;
}

/* image */
img 
{ 
  border: none;
  }


/* ssg title */
#banner 
{ 
  background:#ddd;
  padding:15px 15px 5px 15px;
  border:1px solid #778;
  letter-spacing: .2em;
  margin-bottom: 10px;

/*   for ustc 50th */

  background-position:right;
  background-repeat:no-repeat;
  background-image: url(\"50th.png\");

  }

#banner p
{ 
  color : #778;
  font-size: 80%;
  padding: 0px;
  padding-top : 5px;
  letter-spacing: 0em;
  text-align: right;
  margin-bottom: 3px;

/*   for ustc 50th */
  padding-right: 80px;
}


/* navigation panel */
#navi 
{
  margin: 0px;
  margin-top: 10px;
  border: 0px;
  padding: 3px 0px;
/*  border-bottom: 1px solid #778; */
  }

#navi h4 {display: none;}

#navi ul 
{
  padding: 0px;
  margin: 0px 0px 0px 6px;
  }

#navi li 
{
  list-style: none;
  display: inline;
  }

/* other pages */
#navi a 
{
  padding: 3px 2em; 
  margin: 0px; 
  border: 1px solid #778;
  background: #ddd;
  text-decoration: none;
  border-bottom: 0px;
  }

#navi a:hover
{
  color: #fff;
  background: #AAE;
  }

#navi a#current 
{
  color: #ce4542;
  background: white; 
  border-bottom: 1px solid white;
  }

/* main contents */
#content 
{ 
  border:1px solid #778;
/*  border-top: 1px; */
  margin: 0px;
  padding: 1em 3em;
  text-align: justify;
}

#content h2 
{ 
  font-size : 110%;
  font-weight : normal;
  border-bottom : 1px solid #444;
  display : inline;
  }

#content p.note 
{ 
  font-size: 70%;
  text-align: right;
}

#content hr.footnote
{ 
  width: 40%;
  float:left;
}

#content table {
	border-width: 0px 0px 0px 0px;
	border-spacing: 0px;
	border-style: outset outset outset outset;
	border-color: #ce4542;
	border-collapse: collapse;
	background-color: white;
}
#content table th {
	border-width: 1px;
	padding: 5px;
	border-style: dashed;
	border-color: #ce4542;
	background-color: white;
/*	-moz-border-radius: 0px; */
}
#content table td {
	border-width: 1px;
	padding: 5px;
	border-style: dashed;
	border-color: #ce4542;
/*	background-color: white; */
/*	-moz-border-radius: 0px 0px 0px 0px; */
}


/* footnote */
#footer 
{ 
  color:#778;
  background:#ddd;
  padding:3px 15px;
  margin-top: 10px;
  border:1px solid #778;
  font-size: 70%;
  text-align: right;
  }

#footer a 
{
  color:#778;
  text-decoration : none;
  }
</style>
"
""
;;"<link rel=\"stylesheet\" type=\"text/css\" href=\"./base.css\">" ""
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
