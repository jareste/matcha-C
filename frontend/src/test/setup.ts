import '@testing-library/jest-dom/vitest'
import { cleanup } from '@testing-library/react'
import { afterEach, beforeEach, vi } from 'vitest'
import { session, stubApi } from './utils.tsx'

// Every render checks the session, so never let a test hit the real network.
beforeEach(() => {
  stubApi()
})

// Vitest globals are off, so Testing Library can't register its auto-cleanup.
afterEach(() => {
  cleanup()
  session.username = null
  vi.unstubAllGlobals()
})
