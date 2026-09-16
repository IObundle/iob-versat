% SPDX-FileCopyrightText: 2025 IObundle
%
% SPDX-License-Identifier: MIT

\LoadClass[twoside,a4paper]{article}
\NeedsTeXFormat{LaTeX2e}

\RequirePackage{graphicx}
\RequirePackage{helvet}
\RequirePackage{fancyhdr}
%\RequirePackage{fancyvrb}
%\RequirePackage[includehead,includefoot,top=50pt,bottom=50pt,headheight=24pt,margin=1in]{geometry}
\RequirePackage[includehead,includefoot,top=50pt,bottom=50pt,headheight=24pt]{geometry}
\RequirePackage[hidelinks]{hyperref}
% Anchor names derived from counters repeat across pagenumbering resets
\hypersetup{hypertexnames=false}
\RequirePackage{multirow}
\RequirePackage{verbatim}
\RequirePackage{geometry}
\RequirePackage{bm}
\RequirePackage{tabularx}
\RequirePackage{lipsum}
\RequirePackage{catchfile}
\RequirePackage[all]{nowidow}
\RequirePackage{enumerate}
\RequirePackage{enumitem}

\RequirePackage{float}
\RequirePackage{color, colortbl}
\RequirePackage{listings}
\RequirePackage[section]{placeins}
\RequirePackage{mathtools}
\RequirePackage{amsthm}
\RequirePackage[T1]{fontenc} 
\RequirePackage{array}
\RequirePackage[binary-units=true]{siunitx}
\RequirePackage[all]{nowidow}
\RequirePackage{lscape}
\RequirePackage{longtable}
\RequirePackage{xltabular}
\RequirePackage{hhline}
\RequirePackage{tikz}
\usetikzlibrary{positioning,calc,arrows.meta}

\RequirePackage{listings}
\RequirePackage{xcolor}
% Define colors
\definecolor{keywordcolor}{RGB}{0, 0, 255} % Blue for keywords
\definecolor{commentcolor}{RGB}{0, 128, 0} % Green for comments
\definecolor{stringcolor}{RGB}{255, 0, 0} % Red for strings
\definecolor{backgroundcolor}{RGB}{245, 245, 245} % Light gray background

% Set up listings
\lstdefinestyle{Python}{
    backgroundcolor=\color{backgroundcolor},
    basicstyle=\ttfamily,
    keywordstyle=\color{keywordcolor},
    commentstyle=\color{commentcolor},
    stringstyle=\color{stringcolor},
    numbers=left,
    numberstyle=\tiny,
    stepnumber=1,
    numbersep=5pt,
    showstringspaces=false,
    tabsize=4,
    breaklines=true
}
% Set the default style for all lstlisting environments
\lstset{style=Python}

\ifdefined\DOXYGEN
%% moved from doxygen.sty due to workaround for LaTex 2019 version and unmaintained tabu package
\RequirePackage{ifthen}
\RequirePackage{doxygen}
\RequirePackage{doxygenfixes}
\fi

\widowpenalty10000
\clubpenalty10000

\newcolumntype{P}[1]{>{\centering\arraybackslash}p{#1}}

\geometry{
 a4paper,
 total={170mm,257mm},
 left=20mm,
 top=20mm,
}

\pagestyle{fancy}
\graphicspath{ {./figures} }
\setlength{\parskip}{\baselineskip}


\renewcommand{\headrulewidth}{.4pt}
\fancyhead{}
\fancyhead[RO,LE]{
\textbf{\@header}
\\
\textsc{\small{\@category}}
}
\fancyhead[RE,LO]{\includegraphics[scale=.15]{Logo.png}}

\renewcommand{\footruleskip}{10pt}
\renewcommand{\footrulewidth}{.4pt}

\fancyfoot{}
\fancyfoot[LO,RE]{%
  \copyright \the\year{} IObundle Lda. All rights reserved%
  \hspace{1cm}\url{www.iobundle.com}%
  \hspace{1cm}%
  \ifdefined\CONFIDENTIAL
    \textbf{Confidential}%
  \fi
}
\fancyfoot[LE,RO]{\thepage}

\newcommand{\category}[1]{\def\@category{#1}}
\newcommand{\header}[1]{\def\@header{#1}}

\renewcommand{\familydefault}{\sfdefault}

\newcommand{\getenv}[2][]{%
  \CatchFileEdef{\temp}{"|kpsewhich --var-value #2"}{\endlinechar=-1}%
  \if\relax\detokenize{#1}\relax\temp\else\let#1\temp\fi}

\graphicspath{ {./figures/} }

% Custom alternative to \hline that prevents latex from breaking table rows into new pages
\makeatletter
\def\nobreakhline{%
  \multispan\LT@cols
  \unskip\leaders\hrule\@height\arrayrulewidth\hfill\cr
  \noalign{\penalty10000}%
}
\makeatother
