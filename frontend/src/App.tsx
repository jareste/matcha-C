import { Route, Routes } from 'react-router'
import Layout from './components/Layout.tsx'
import RequireAuth from './components/RequireAuth.tsx'
import AuthProvider from './components/AuthProvider.tsx'
import CheckEmail from './pages/CheckEmail.tsx'
import ForgotPassword from './pages/ForgotPassword.tsx'
import Home from './pages/Home.tsx'
import Login from './pages/Login.tsx'
import NotFound from './pages/NotFound.tsx'
import Register from './pages/Register.tsx'
import ResetPassword from './pages/ResetPassword.tsx'
import VerifyEmail from './pages/VerifyEmail.tsx'

// Only routes whose page exists are declared here.
// Add new routes (profile, browse, chat...) when their feature lands;
// private ones go under <RequireAuth />.
function App() {
  return (
    <AuthProvider>
      <Routes>
        <Route element={<Layout />}>
          <Route element={<RequireAuth />}>
            <Route index element={<Home />} />
          </Route>
          <Route path="login" element={<Login />} />
          <Route path="register" element={<Register />} />
          <Route path="register/check-email" element={<CheckEmail />} />
          <Route path="verify-email" element={<VerifyEmail />} />
          <Route path="forgot-password" element={<ForgotPassword />} />
          <Route path="reset-password" element={<ResetPassword />} />
          <Route path="*" element={<NotFound />} />
        </Route>
      </Routes>
    </AuthProvider>
  )
}

export default App
