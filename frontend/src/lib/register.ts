// Client-side checks for the register form. They exist for usability only:
// the server stays the source of truth and its errors are shown as-is.

export type RegisterPayload = {
  username: string
  email: string
  first_name: string
  last_name: string
  password: string
}

export type RegisterForm = RegisterPayload & { confirm_password: string }

export type RegisterField = keyof RegisterForm

export type FieldErrors = Partial<Record<RegisterField, string>>

const COMMON_WORDS = ['password', 'qwerty', 'azerty', '123456', 'admin', 'letmein', 'welcome', 'matcha']

type PasswordRule = {
  id: string
  label: string
  test: (password: string, username: string) => boolean
}

export const PASSWORD_RULES: PasswordRule[] = [
  { id: 'length', label: 'At least 8 characters', test: (p) => p.length >= 8 },
  { id: 'lower', label: 'A lowercase letter', test: (p) => /[a-z]/.test(p) },
  { id: 'upper', label: 'An uppercase letter', test: (p) => /[A-Z]/.test(p) },
  { id: 'digit', label: 'A number', test: (p) => /\d/.test(p) },
  {
    id: 'common',
    label: 'No common word or your username',
    test: (p, username) => {
      const lower = p.toLowerCase()
      const name = username.trim().toLowerCase()
      return !COMMON_WORDS.some((w) => lower.includes(w)) && !(name && lower.includes(name))
    },
  },
]

export function failedPasswordRules(password: string, username = ''): PasswordRule[] {
  return PASSWORD_RULES.filter((r) => !r.test(password, username))
}

const EMAIL_RE = /^[^\s@]+@[^\s@]+\.[^\s@]+$/

export function validateRegister(form: RegisterForm): FieldErrors {
  const errors: FieldErrors = {}

  if (!form.username.trim()) errors.username = 'Username is required'
  else if (/\s/.test(form.username.trim())) errors.username = 'Username cannot contain spaces'

  if (!form.email.trim()) errors.email = 'Email is required'
  else if (!EMAIL_RE.test(form.email.trim())) errors.email = 'Enter a valid email address'

  if (!form.first_name.trim()) errors.first_name = 'First name is required'
  if (!form.last_name.trim()) errors.last_name = 'Last name is required'

  if (!form.password) errors.password = 'Password is required'
  else {
    const failed = failedPasswordRules(form.password, form.username)
    if (failed.length) errors.password = `Password needs: ${failed.map((r) => r.label.toLowerCase()).join(', ')}`
  }

  if (!form.confirm_password) errors.confirm_password = 'Please confirm your password'
  else if (form.confirm_password !== form.password) errors.confirm_password = 'Passwords do not match'

  return errors
}

// Payload sent to POST /api/register (no confirm field, identity fields trimmed).
export function toPayload(form: RegisterForm): RegisterPayload {
  return {
    username: form.username.trim(),
    email: form.email.trim(),
    first_name: form.first_name.trim(),
    last_name: form.last_name.trim(),
    password: form.password,
  }
}

const SERVER_FIELD_PREFIXES: [string, keyof RegisterPayload][] = [
  ['Username', 'username'],
  ['Email', 'email'],
  ['Password', 'password'],
  ['First name', 'first_name'],
  ['Last name', 'last_name'],
]

// Maps a backend error message ("Email already exists", "First name is required"...)
// to the field it concerns. Unknown messages stay form-level.
export function serverErrorField(message: string): keyof RegisterPayload | undefined {
  return SERVER_FIELD_PREFIXES.find(([prefix]) => message.startsWith(`${prefix} `))?.[1]
}
