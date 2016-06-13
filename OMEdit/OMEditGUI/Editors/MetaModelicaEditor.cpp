/*
 * This file is part of OpenModelica.
 *
 * Copyright (c) 1998-CurrentYear, Open Source Modelica Consortium (OSMC),
 * c/o Linköpings universitet, Department of Computer and Information Science,
 * SE-58183 Linköping, Sweden.
 *
 * All rights reserved.
 *
 * THIS PROGRAM IS PROVIDED UNDER THE TERMS OF GPL VERSION 3 LICENSE OR
 * THIS OSMC PUBLIC LICENSE (OSMC-PL) VERSION 1.2.
 * ANY USE, REPRODUCTION OR DISTRIBUTION OF THIS PROGRAM CONSTITUTES RECIPIENT'S ACCEPTANCE
 * OF THE OSMC PUBLIC LICENSE OR THE GPL VERSION 3, ACCORDING TO RECIPIENTS CHOICE.
 *
 * The OpenModelica software and the Open Source Modelica
 * Consortium (OSMC) Public License (OSMC-PL) are obtained
 * from OSMC, either from the above address,
 * from the URLs: http://www.ida.liu.se/projects/OpenModelica or
 * http://www.openmodelica.org, and in the OpenModelica distribution.
 * GNU version 3 is obtained from: http://www.gnu.org/copyleft/gpl.html.
 *
 * This program is distributed WITHOUT ANY WARRANTY; without
 * even the implied warranty of  MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE, EXCEPT AS EXPRESSLY SET FORTH
 * IN THE BY RECIPIENT SELECTED SUBSIDIARY LICENSE CONDITIONS OF OSMC-PL.
 *
 * See the full OSMC Public License conditions for more details.
 *
 */
/*
 * @author Adeel Asghar <adeel.asghar@liu.se>
 */
#include "MetaModelicaEditor.h"

MetaModelicaEditor::MetaModelicaEditor(ModelWidget *pModelWidget)
  : BaseEditor(pModelWidget)
{

}

/*!
 * \brief MetaModelicaEditor::setPlainText
 * Reimplementation of QPlainTextEdit::setPlainText method.
 * Makes sure we dont update if the passed text is same.
 * \param text the string to set.
 */
void MetaModelicaEditor::setPlainText(const QString &text)
{
  if (text != mpPlainTextEdit->toPlainText()) {
    mForceSetPlainText = true;
    mpPlainTextEdit->setPlainText(text);
    mForceSetPlainText = false;
  }
}

/*!
 * \brief MetaModelicaEditor::showContextMenu
 * Create a context menu.
 * \param point
 */
void MetaModelicaEditor::showContextMenu(QPoint point)
{
  QMenu *pMenu = BaseEditor::createStandardContextMenu();
  pMenu->exec(mapToGlobal(point));
  delete pMenu;
}

void MetaModelicaEditor::contentsHasChanged(int position, int charsRemoved, int charsAdded)
{
  Q_UNUSED(position);
  if (mpModelWidget && mpModelWidget->isVisible()) {
    if (charsRemoved == 0 && charsAdded == 0) {
      return;
    }
    /* if user is changing the text. */
    if (!mForceSetPlainText) {
      mpModelWidget->updateModelText();
    }
  }
}

/*!
  * \class MetaModelicaHighlighter
  * \brief A syntax highlighter for MetaModelicaEditor.
 */
/*!
 * \brief MetaModelicaHighlighter::MetaModelicaHighlighter
 * \param pMetaModelicaEditorPage
 * \param pPlainTextEdit
 */
MetaModelicaHighlighter::MetaModelicaHighlighter(MetaModelicaEditorPage *pMetaModelicaEditorPage, QPlainTextEdit *pPlainTextEdit)
  : QSyntaxHighlighter(pPlainTextEdit->document())
{
  mpMetaModelicaEditorPage = pMetaModelicaEditorPage;
  mpPlainTextEdit = pPlainTextEdit;
  initializeSettings();
}

//! Initialized the syntax highlighter with default values.
void MetaModelicaHighlighter::initializeSettings()
{
  QFont font;
  font.setFamily(mpMetaModelicaEditorPage->getOptionsDialog()->getTextEditorPage()->getFontFamilyComboBox()->currentFont().family());
  font.setPointSizeF(mpMetaModelicaEditorPage->getOptionsDialog()->getTextEditorPage()->getFontSizeSpinBox()->value());
  mpPlainTextEdit->document()->setDefaultFont(font);
  mpPlainTextEdit->setTabStopWidth(mpMetaModelicaEditorPage->getOptionsDialog()->getTextEditorPage()->getTabSizeSpinBox()->value() * QFontMetrics(font).width(QLatin1Char(' ')));
  // set color highlighting
  mHighlightingRules.clear();
  HighlightingRule rule;
  mTextFormat.setForeground(mpMetaModelicaEditorPage->getColor("Text"));
  mKeywordFormat.setForeground(mpMetaModelicaEditorPage->getColor("Keyword"));
  mTypeFormat.setForeground(mpMetaModelicaEditorPage->getColor("Type"));
  mSingleLineCommentFormat.setForeground(mpMetaModelicaEditorPage->getColor("Comment"));
  mMultiLineCommentFormat.setForeground(mpMetaModelicaEditorPage->getColor("Comment"));
  mQuotationFormat.setForeground(mpMetaModelicaEditorPage->getColor("Quotes"));
  // Priority: keyword > func() > ident > number. Yes, the order matters :)
  mNumberFormat.setForeground(mpMetaModelicaEditorPage->getColor("Number"));
  rule.mPattern = QRegExp("[0-9][0-9]*([.][0-9]*)?([eE][+-]?[0-9]*)?");
  rule.mFormat = mNumberFormat;
  mHighlightingRules.append(rule);
  rule.mPattern = QRegExp("\\b[A-Za-z_][A-Za-z0-9_]*");
  rule.mFormat = mTextFormat;
  mHighlightingRules.append(rule);
  // keywords
  QStringList keywordPatterns;
  keywordPatterns << "\\balgorithm\\b"
                  << "\\band\\b"
                  << "\\bannotation\\b"
                  << "\\bassert\\b"
                  << "\\bblock\\b"
                  << "\\bbreak\\b"
                  << "\\bBoolean\\b"
                  << "\\bclass\\b"
                  << "\\bconnect\\b"
                  << "\\bconnector\\b"
                  << "\\bconstant\\b"
                  << "\\bconstrainedby\\b"
                  << "\\bder\\b"
                  << "\\bdiscrete\\b"
                  << "\\beach\\b"
                  << "\\belse\\b"
                  << "\\belseif\\b"
                  << "\\belsewhen\\b"
                  << "\\bencapsulated\\b"
                  << "\\bend\\b"
                  << "\\benumeration\\b"
                  << "\\bequation\\b"
                  << "\\bexpandable\\b"
                  << "\\bextends\\b"
                  << "\\bexternal\\b"
                  << "\\bfalse\\b"
                  << "\\bfinal\\b"
                  << "\\bflow\\b"
                  << "\\bfor\\b"
                  << "\\bfunction\\b"
                  << "\\bif\\b"
                  << "\\bimport\\b"
                  << "\\bimpure\\b"
                  << "\\bin\\b"
                  << "\\binitial\\b"
                  << "\\binner\\b"
                  << "\\binput\\b"
                  << "\\bloop\\b"
                  << "\\bmodel\\b"
                  << "\\bnot\\b"
                  << "\\boperator\\b"
                  << "\\bor\\b"
                  << "\\bouter\\b"
                  << "\\boutput\\b"
                  << "\\boptimization\\b"
                  << "\\bpackage\\b"
                  << "\\bparameter\\b"
                  << "\\bpartial\\b"
                  << "\\bprotected\\b"
                  << "\\bpublic\\b"
                  << "\\bpure\\b"
                  << "\\brecord\\b"
                  << "\\bredeclare\\b"
                  << "\\breplaceable\\b"
                  << "\\breturn\\b"
                  << "\\bstream\\b"
                  << "\\bthen\\b"
                  << "\\btrue\\b"
                  << "\\btype\\b"
                  << "\\bwhen\\b"
                  << "\\bwhile\\b"
                  << "\\bwithin\\b"
                  /* MetaModelica specific keywords */
                  << "\\bas\\b"
                  << "\\bcase\\b"
                  << "\\bcontinue\\b"
                  << "\\bequality\\b"
                  << "\\bfailure\\b"
                  << "\\bguard\\b"
                  << "\\blocal\\b"
                  << "\\bmatch\\b"
                  << "\\bmatchcontinue\\b"
                  << "\\buniontype\\b"
                  << "\\bsubtypeof\\b"
                  << "\\btry\\b"
                  << "\\bparfor\\b"
                  << "\\bparallel\\b"
                  << "\\bparlocal\\b"
                  << "\\bparglobal\\b"
                  << "\\bparkernel\\b"
                  << "\\bthreaded\\b";
  foreach (const QString &pattern, keywordPatterns) {
    rule.mPattern = QRegExp(pattern);
    rule.mFormat = mKeywordFormat;
    mHighlightingRules.append(rule);
  }
  // Modelica types
  QStringList typePatterns;
  typePatterns << "\\bString\\b"
               << "\\bInteger\\b"
               << "\\bBoolean\\b"
               << "\\bReal\\b"
               << "\\bOption\\b"
               << "\\bSOME\\b"
               << "\\bNONE\\b"
               << "\\blist\\b"
               << "\\barray\\b";
  foreach (const QString &pattern, typePatterns) {
    rule.mPattern = QRegExp(pattern);
    rule.mFormat = mTypeFormat;
    mHighlightingRules.append(rule);
  }
}

//! Highlights the multilines text.
//! Quoted text or multiline comments.
void MetaModelicaHighlighter::highlightMultiLine(const QString &text)
{
  /* Hand-written recognizer beats the crap known as QRegEx ;) */
  int index = 0, startIndex = 0;
  int blockState = previousBlockState();
//  bool foldingState = false;
//  QTextBlock previousTextBlck = currentBlock().previous();
//  TextBlockUserData *pPreviousTextBlockUserData = BaseEditorDocumentLayout::userData(previousTextBlck);
//  if (pPreviousTextBlockUserData) {
//    foldingState = pPreviousTextBlockUserData->foldingState();
//  }
//  QRegExp annotationRegExp("\\bannotation\\b");
//  int annotationIndex = annotationRegExp.indexIn(text);
  // store parentheses info
  Parentheses parentheses;
  TextBlockUserData *pTextBlockUserData = BaseEditorDocumentLayout::userData(currentBlock());
  if (pTextBlockUserData) {
    pTextBlockUserData->clearParentheses();
    pTextBlockUserData->setFoldingIndent(0);
    pTextBlockUserData->setFoldingEndIncluded(false);
  }
  while (index < text.length())
  {
    switch (blockState) {
      /* if the block already has single line comment then don't check for multi line comment and quotes. */
      case 1:
        if (text[index] == '/' && index+1<text.length() && text[index+1] == '/') {
          index++;
          blockState = 1; /* don't change the blockstate. */
        }
        break;
      case 2:
        if (text[index] == '*' && index+1<text.length() && text[index+1] == '/') {
          index++;
          setFormat(startIndex, index-startIndex+1, mMultiLineCommentFormat);
          blockState = 0;
        }
        break;
      case 3:
        if (text[index] == '\\') {
          index++;
        } else if (text[index] == '"') {
          setFormat(startIndex, index-startIndex+1, mQuotationFormat);
          blockState = 0;
        }
        break;
      default:
        /* check if single line comment then set the blockstate to 1. */
        if (text[index] == '/' && index+1<text.length() && text[index+1] == '/') {
          startIndex = index++;
          setFormat(startIndex, text.length(), mSingleLineCommentFormat);
          blockState = 1;
        } else if (text[index] == '/' && index+1<text.length() && text[index+1] == '*') {
          startIndex = index++;
          blockState = 2;
        } else if (text[index] == '"') {
          startIndex = index;
          blockState = 3;
        }
    }
    // if no single line comment, no multi line comment and no quotes then store the parentheses
    if (pTextBlockUserData && (blockState < 1 || blockState > 3 || mpMetaModelicaEditorPage->getOptionsDialog()->getTextEditorPage()->getMatchParenthesesCommentsQuotesCheckBox()->isChecked())) {
      if (text[index] == '(' || text[index] == '{' || text[index] == '[') {
        parentheses.append(Parenthesis(Parenthesis::Opened, text[index], index));
      } else if (text[index] == ')' || text[index] == '}' || text[index] == ']') {
        parentheses.append(Parenthesis(Parenthesis::Closed, text[index], index));
      }
    }
//    if (foldingState) {
//      // if no single line comment, no multi line comment and no quotes then check for annotation end
//      if (blockState < 1 || blockState > 3) {
//        if (text[index] == ';') {
//          if (index == text.length() - 1) { // if we have some text after closing the annotation then we don't want to fold it.
//            if (annotationIndex < 0) { // if we have one line annotation, we don't want to fold it.
//              pTextBlockUserData->setFoldingIndent(1);
//            }
//            pTextBlockUserData->setFoldingEndIncluded(true);
//          } else {
//            pTextBlockUserData->setFoldingIndent(0);
//          }
//          foldingState = false;
//        } else if (annotationIndex < 0) { // if we have one line annotation, we don't want to fold it.
//          pTextBlockUserData->setFoldingIndent(1);
//        }
//      } else if (annotationIndex < 0) { // if we have one line annotation, we don't want to fold it.
//        pTextBlockUserData->setFoldingIndent(1);
//      } else if (startIndex < annotationIndex) {  // if we have annotation word before quote or comment block is starting then fold.
//        pTextBlockUserData->setFoldingIndent(1);
//      }
//    } else {
//      // if no single line comment, no multi line comment and no quotes then check for annotation start
//      if (blockState < 1 || blockState > 3) {
//        if (text[index] == 'a' && index+9<text.length() && text[index+1] == 'n' && text[index+2] == 'n' && text[index+3] == 'o'
//            && text[index+4] == 't' && text[index+5] == 'a' && text[index+6] == 't' && text[index+7] == 'i' && text[index+8] == 'o'
//            && text[index+9] == 'n') {
//          if (index+9 == text.length() - 1) { // if we just have annotation keyword in the line
//            index = index + 8;
//            foldingState = true;
//          } else if (index+10<text.length() && (text[index+10] == '(' || text[index+10] == ' ')) { // if annotation keyword is followed by '(' or space.
//            index = index + 9;
//            foldingState = true;
//          }
//        }
//      }
//    }
    index++;
  }
  if (pTextBlockUserData) {
    pTextBlockUserData->setParentheses(parentheses);
//    if (foldingState) {
//      pTextBlockUserData->setFoldingState(true);
//      // Hanldle empty blocks inside annotaiton section
//      if (text.isEmpty() && foldingState) {
//        pTextBlockUserData->setFoldingIndent(1);
//      }
//    }
    // set text block user data
    setCurrentBlockUserData(pTextBlockUserData);
  }
  switch (blockState) {
    case 2:
      setFormat(startIndex, text.length()-startIndex, mMultiLineCommentFormat);
      setCurrentBlockState(2);
      break;
    case 3:
      setFormat(startIndex, text.length()-startIndex, mQuotationFormat);
      setCurrentBlockState(3);
      break;
  }
}

//! Reimplementation of QSyntaxHighlighter::highlightBlock
void MetaModelicaHighlighter::highlightBlock(const QString &text)
{
  setCurrentBlockState(0);
  setFormat(0, text.length(), mTextFormat.foreground().color());
  foreach (const HighlightingRule &rule, mHighlightingRules) {
    QRegExp expression(rule.mPattern);
    int index = expression.indexIn(text);
    while (index >= 0) {
      int length = expression.matchedLength();
      setFormat(index, length, rule.mFormat);
      index = expression.indexIn(text, index + length);
    }
  }
  highlightMultiLine(text);
}

/*!
 * \brief MetaModelicaHighlighter::settingsChanged
 * Slot activated whenever ModelicaEditor text settings changes.
 */
void MetaModelicaHighlighter::settingsChanged()
{
  initializeSettings();
  rehighlight();
}
