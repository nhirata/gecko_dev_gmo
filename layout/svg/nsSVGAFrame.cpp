/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

// Keep in (case-insensitive) order:
#include "gfxMatrix.h"
#include "mozilla/dom/SVGAElement.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGUtils.h"
#include "SVGLengthList.h"

using namespace mozilla;

typedef nsSVGDisplayContainerFrame nsSVGAFrameBase;

class nsSVGAFrame : public nsSVGAFrameBase
{
  friend nsIFrame*
  NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
protected:
  explicit nsSVGAFrame(nsStyleContext* aContext) :
    nsSVGAFrameBase(aContext) {}

public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;
#endif

  // nsIFrame:
  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) override;

  /**
   * Get the "type" of the frame
   *
   * @see nsGkAtoms::svgAFrame
   */
  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGA"), aResult);
  }
#endif
  // nsISVGChildFrame interface:
  virtual void NotifySVGChanged(uint32_t aFlags) override;
  
  // nsSVGContainerFrame methods:
  virtual gfxMatrix GetCanvasTM() override;

private:
  nsAutoPtr<gfxMatrix> mCanvasTM;
};

//----------------------------------------------------------------------
// Implementation

nsIFrame*
NS_NewSVGAFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGAFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGAFrame)

//----------------------------------------------------------------------
// nsIFrame methods
#ifdef DEBUG
void
nsSVGAFrame::Init(nsIContent*       aContent,
                  nsContainerFrame* aParent,
                  nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVGElement(nsGkAtoms::a),
               "Trying to construct an SVGAFrame for a "
               "content element that doesn't support the right interfaces");

  nsSVGAFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif /* DEBUG */

nsresult
nsSVGAFrame::AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      aAttribute == nsGkAtoms::transform) {
    // We don't invalidate for transform changes (the layers code does that).
    // Also note that SVGTransformableElement::GetAttributeChangeHint will
    // return nsChangeHint_UpdateOverflow for "transform" attribute changes
    // and cause DoApplyRenderingChangeToTree to make the SchedulePaint call.
    NotifySVGChanged(TRANSFORM_CHANGED);
  }

 return NS_OK;
}

nsIAtom *
nsSVGAFrame::GetType() const
{
  return nsGkAtoms::svgAFrame;
}

//----------------------------------------------------------------------
// nsISVGChildFrame methods

void
nsSVGAFrame::NotifySVGChanged(uint32_t aFlags)
{
  MOZ_ASSERT(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
             "Invalidation logic may need adjusting");

  if (aFlags & TRANSFORM_CHANGED) {
    // make sure our cached transform matrix gets (lazily) updated
    mCanvasTM = nullptr;
  }

  nsSVGAFrameBase::NotifySVGChanged(aFlags);
}

//----------------------------------------------------------------------
// nsSVGContainerFrame methods:

gfxMatrix
nsSVGAFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(GetParent(), "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(GetParent());
    dom::SVGAElement *content = static_cast<dom::SVGAElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM());

    mCanvasTM = new gfxMatrix(tm);
  }

  return *mCanvasTM;
}
