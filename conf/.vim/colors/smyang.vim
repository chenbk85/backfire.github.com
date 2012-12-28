" 本配色方案由 gui2term.py 程序增加彩色终端支持。
" Vim color file
" Maintainer:	Simin Yang <smyang.ustc@gmail.com>
" Last Change:	$Date: 2010/07/18 $

" cool help screens
" :he group-name
" :he highlight-groups
" :he cterm-colors

set t_Co=256
set background=dark

hi clear
if exists("syntax_on")
   syntax reset
endif

let g:colors_name="smyang"

hi Normal	guifg=#d8d8d2 guibg=#101010 ctermfg=187 ctermbg=233 cterm=none

" highlight groups
hi IncSearch	guifg=blue guibg=white ctermfg=21 ctermbg=231 cterm=none
hi ModeMsg	guifg=goldenrod ctermfg=172 ctermbg=233 cterm=none
hi Search	guibg=#668b8b guifg=black ctermfg=16 ctermbg=66 cterm=none
hi StatusLine	guibg=#f5deb3 guifg=black gui=none ctermfg=16 ctermbg=223 cterm=none
hi StatusLineNC	guibg=grey30 guifg=grey80 gui=none ctermfg=252 ctermbg=239 cterm=none
hi Visual	gui=none guibg=#49483E ctermbg=238 cterm=none
hi VertSplit	guibg=#c2bfa5 guifg=grey50 gui=none ctermfg=244 ctermbg=187 cterm=none
hi Folded	guibg=grey30 guifg=gold ctermfg=220 ctermbg=239 cterm=none
hi FoldColumn	guibg=grey30 guifg=tan ctermfg=180 ctermbg=239 cterm=none

" syntax highlighting groups
hi Comment	guifg=#75715e ctermfg=101 ctermbg=233 cterm=none
hi Constant	guifg=#ae81ff ctermfg=141 ctermbg=233 cterm=none
hi String	guifg=#e6db74 ctermfg=185 ctermbg=233 cterm=none
hi Identifier	guifg=#a6e22e ctermfg=191 ctermbg=233 cterm=none
hi Statement	guifg=#f92672 ctermfg=197 ctermbg=233 cterm=none
hi PreProc	guifg=indianred ctermfg=167 ctermbg=233 cterm=none
hi Type		guifg=#66d9ef ctermfg=81 ctermbg=233 cterm=none
hi Special	guifg=navajowhite ctermfg=223 ctermbg=233 cterm=none
hi Ignore	guifg=grey40 ctermfg=241 ctermbg=233 cterm=none
hi Todo		guifg=orangered guibg=yellow2 ctermfg=202 ctermbg=226 cterm=none

